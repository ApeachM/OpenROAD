///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "mdm/MultiDieManager.hh"
#include "odb/db.h"

namespace mdm {
using namespace std;
void SwitchInstanceHelper::switchInstanceToAssignedDie(
    MultiDieManager* manager,
    odb::dbInst* originalInst)
{
  auto targetBlockID = findAssignedDieId(originalInst);
  auto [targetBlock, targetLib] = findTargetDieAndLib(manager, targetBlockID);
  auto targetMaster
      = targetLib->findMaster(originalInst->getMaster()->getName().c_str());

  if (!targetMaster->getSite()) {
    targetMaster->setSite((*targetBlock->getRows().begin())->getSite());
  }

  auto newInst = odb::dbInst::create(
      targetBlock, targetMaster, originalInst->getName().c_str());
  inheritPlacementInfo(originalInst, newInst);
  inheritNetInfo(originalInst, newInst);
  inheritProperties(originalInst, newInst);
  inheritGroupInfo(originalInst, newInst);

  odb::dbInst::destroy(originalInst);
}

int SwitchInstanceHelper::findAssignedDieId(odb::dbInst* inst)
{
  auto partitionInfo = odb::dbIntProperty::find(inst, "partition_id");
  auto dieId = partitionInfo->getValue();
  return dieId;
}

std::pair<odb::dbBlock*, odb::dbLib*> SwitchInstanceHelper::findTargetDieAndLib(
    MultiDieManager* manager,
    int dieID)
{
  odb::dbBlock* targetBlock = nullptr;
  odb::dbLib* targetLib = nullptr;

  auto blockIter = manager->db_->getChip()->getBlock()->getChildren().begin();
  for (int i = 0; i < dieID; ++i) {
    blockIter++;
  }
  targetBlock = *blockIter;

  auto libIter = manager->db_->getLibs().begin();
  libIter++;  // skip the top hier lib
  for (int i = 0; i < dieID; ++i) {
    libIter++;
  }
  targetLib = *libIter;
  auto targetlibName = targetLib->getName();

  return {targetBlock, targetLib};
}
void SwitchInstanceHelper::inheritPlacementInfo(odb::dbInst* originalInst,
                                                odb::dbInst* newInst)
{
  auto placementStatus = originalInst->getPlacementStatus();
  if (placementStatus == odb::dbPlacementStatus::FIRM) {
    newInst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  } else {
    newInst->setPlacementStatus(placementStatus);
  }
  if (placementStatus != odb::dbPlacementStatus::NONE) {
    odb::Point location = originalInst->getLocation();
    newInst->setLocation(location.x(), location.y());
  }
}
void SwitchInstanceHelper::inheritNetInfo(odb::dbInst* originalInst,
                                          odb::dbInst* newInst)
{
  vector<pair<string, odb::dbNet*>> netInfoContainer;
  auto targetBlock = newInst->getBlock();
  for (auto iTerm : originalInst->getITerms()) {
    if (!iTerm->getNet()) {
      continue;
    }
    auto pinName = iTerm->getMTerm()->getName();
    auto connectedNet = iTerm->getNet();
    netInfoContainer.emplace_back(pinName, connectedNet);
  }
  for (const auto& netInfo : netInfoContainer) {
    auto iTerm = newInst->findITerm(netInfo.first.c_str());
    auto originalNet = netInfo.second;
    assert(iTerm);

    // find the net in the target block
    // if it doesn't exist, then make the new one
    auto newNet = targetBlock->findNet(originalNet->getName().c_str());
    if (!newNet) {
      newNet = odb::dbNet::create(targetBlock, originalNet->getName().c_str());
      assert(newNet);
    }
    iTerm->connect(newNet);
  }
}
void SwitchInstanceHelper::inheritProperties(odb::dbInst* originalInst,
                                             odb::dbInst* newInst)
{
  struct PropertyStorage
  {
    vector<int> intProperties;
    vector<string> stringProperties;
    vector<double> doubleProperties;
    vector<bool> boolProperties;
    vector<string> nameOfIntProperties;
    vector<string> nameOfStringProperties;
    vector<string> nameOfDoubleProperties;
    vector<string> nameOfBoolProperties;
  };
  PropertyStorage propertyStorage;
  for (auto property : odb::dbProperty::getProperties(originalInst)) {
    if (property->getType() == odb::dbProperty::INT_PROP) {
      propertyStorage.intProperties.push_back(
          odb::dbIntProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      propertyStorage.nameOfIntProperties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::STRING_PROP) {
      propertyStorage.stringProperties.push_back(
          odb::dbStringProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      propertyStorage.nameOfStringProperties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::DOUBLE_PROP) {
      propertyStorage.doubleProperties.push_back(
          odb::dbDoubleProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      propertyStorage.nameOfDoubleProperties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::BOOL_PROP) {
      propertyStorage.boolProperties.push_back(
          odb::dbBoolProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      propertyStorage.nameOfBoolProperties.push_back(property->getName());
    }
  }
  for (int i = 0; i < propertyStorage.intProperties.size(); ++i) {
    odb::dbIntProperty::create(
        newInst,
        propertyStorage.nameOfIntProperties.at(i).c_str(),
        propertyStorage.intProperties.at(i));
  }
  for (int i = 0; i < propertyStorage.stringProperties.size(); ++i) {
    odb::dbStringProperty::create(
        newInst,
        propertyStorage.nameOfStringProperties.at(i).c_str(),
        propertyStorage.stringProperties.at(i).c_str());
  }
  for (int i = 0; i < propertyStorage.doubleProperties.size(); ++i) {
    odb::dbDoubleProperty::create(
        newInst,
        propertyStorage.nameOfDoubleProperties.at(i).c_str(),
        propertyStorage.doubleProperties.at(i));
  }
  for (int i = 0; i < propertyStorage.boolProperties.size(); ++i) {
    odb::dbBoolProperty::create(
        newInst,
        propertyStorage.nameOfBoolProperties.at(i).c_str(),
        propertyStorage.boolProperties.at(i));
  }
}
void SwitchInstanceHelper::inheritGroupInfo(odb::dbInst* originalInst,
                                            odb::dbInst* newInst)
{
  odb::dbGroup* group = originalInst->getGroup();
  if (!group) {
    return;
  }
  group->addInst(newInst);
}
}  // namespace mdm