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

#include "SemiLegalizer.h"
#include "TestCaseManager.h"
// #include "db_sta/dbNetwork.hh"
// #include "db_sta/dbSta.hh"
// #include "dpl/Opendp.h"
// #include "dpo/Optdp.h"
// #include "gpl/Replace.h"
// #include "odb/db.h"
// #include "par/PartitionMgr.h"
// #include "utl/Logger.h"

namespace mdm {
using namespace std;

MultiDieManager::MultiDieManager() = default;

MultiDieManager::~MultiDieManager() = default;

void MultiDieManager::init(odb::dbDatabase* db,
                           utl::Logger* logger,
                           par::PartitionMgr* partitionMgr,
                           gpl::Replace* replace,
                           dpl::Opendp* opendp,
                           sta::dbSta* sta)
{
  db_ = db;
  logger_ = logger;
  partitionMgr_ = partitionMgr;
  replace_ = replace;
  opendp_ = opendp;
  sta_ = sta;

  testCaseManager_ = new TestCaseManager();
  testCaseManager_->setMDM(this);
}

void MultiDieManager::set3DIC(int numberOfDie, float areaRatio)
{
  testCaseManager_->setMDM(this);

  numberOfDie_ = numberOfDie;
  shrinkAreaRatio_ = areaRatio;

  logger_->info(utl::MDM, 1, "Set number of die to {}", numberOfDie_);
  // Set up for multi dies
  splitInstances();
}

void MultiDieManager::splitInstances()
{
  const char* partitionFileName = partitionFile_.c_str();
  readPartitionInfo(partitionFileName);
  makeSubBlocks();

  switchInstancesToAssignedDie();

  // make the interconnections between (m) and (m+1)th blocks
  auto blocks = db_->getChip()->getBlock()->getChildren();
  auto iter = blocks.begin();
  odb::dbBlock* lowerBlock = *iter;
  for (++iter; iter != blocks.end(); ++iter) {
    odb::dbBlock* upperBlock = *iter;
    makeInterconnections(lowerBlock, upperBlock);
    lowerBlock = upperBlock;
  }

  // make the connection between the block terminals in the top hier
  // and the child block
  makeIOPinInterconnections();

  // remove the redundant nets (floating nets)
  for (auto net : db_->getChip()->getBlock()->getNets()) {
    if (net->getBTermCount() == 0 && net->getITermCount() == 0) {
      odb::dbNet::destroy(net);
    }
  }
}

void MultiDieManager::makeSubBlocks()
{
  assert(db_->getTechs().size() >= 2);

  odb::dbBlock* topBlock = db_->getChip()->getBlock();

  // Let's assume that the die area are same each other
  auto dieArea = topBlock->getDieArea();

  auto techIter = db_->getTechs().begin();
  techIter++;
  for (int dieIdx = 0; dieIdx < numberOfDie_; ++dieIdx) {
    string dieName = "Die" + to_string(dieIdx + 1);
    auto childBlock
        = odb::dbBlock::create(topBlock, dieName.c_str(), *techIter++);
    odb::dbInst::create(topBlock, childBlock, dieName.c_str());
    childBlock->setDieArea(dieArea);
    if (!testCaseManager_->isICCADParsed()) {
      inheritRows(topBlock, childBlock);
    }
  }
  if (testCaseManager_->isICCADParsed()) {
    testCaseManager_->rowConstruction();
  }
}
void MultiDieManager::switchInstancesToAssignedDie()
{
  // We will switch the instances from top hier die to assigned child die.
  auto topBlock = db_->getChip()->getBlock();

  // Because in the instances will be destroyed in
  // `switchInstanceToAssignedDie`, we need to make the copy of the pointer of
  // the instances in advances.
  vector<odb::dbInst*> instSet;
  for (auto inst : topBlock->getInsts()) {
    if (inst->getChild()) {
      continue;
    }
    instSet.push_back(inst);
  }

  // switch the instances one by one
  for (auto inst : instSet) {
    if (!odb::dbIntProperty::find(inst, "partition_id")) {
      logger_->warn(
          utl::MDM,
          0,
          "The " + inst->getName() + " doesn't have the partition id.");
    } else {
      SwitchInstanceHelper::switchInstanceToAssignedDie(this, inst);
    }
  }
}

void MultiDieManager::makeInterconnections(odb::dbBlock* lowerBlock,
                                           odb::dbBlock* upperBlock)
{
  // REMINDER
  // Current State, specifically for the lower die:
  // Let `instForLowerBlock` as the instance in the top heir block
  // and exist for representing the lower block.
  // e.g.
  // auto instForLowerBlock = lowerBlock->getParentInst();
  // auto instForUpperBlock = upperBlock->getParentInst();
  //
  // [1] About `lowerBlockTerm`
  // 1.1. This is in the `lowerBlock`,
  // 1.2. This is connected to `lowerBlockNet`.
  //
  // [2] About The new iTerm of `instForLowerBlock`
  // 2.1. This is in the top heir block, not in `lowerBlock`.
  // 2.2. This is mapped to the `lowerBlockTerm`, which is in `lowerBlock`.
  // 2.3. This is accessed by using
  // `instForLowerBlock->findITerm(interconnectName.c_str())`.
  // 2.4. This is accessed by using`lowerBlockTerm->getITerm()`.
  // 2.5. This is not connected to any net.

  // interconnections for
  // child block1 - top heir block - child block2

  auto topHeirBlock = db_->getChip()->getBlock();
  int interconnectionNum = 0;
  // traverse the nets in the lower block
  for (auto lowerBlockNet : lowerBlock->getNets()) {
    auto netName = lowerBlockNet->getName();
    auto upperBlockNet = upperBlock->findNet(netName.c_str());
    if (!upperBlockNet) {
      continue;
    }
    auto topHierNet = topHeirBlock->findNet(netName.c_str());
    assert(topHierNet);

    // connect between topHeir - lowerBlock, and between topHeir - upperBlock
    auto interconnectName = netName + "Interconnection";
    auto lowerBlockTerm
        = odb::dbBTerm::create(lowerBlockNet, interconnectName.c_str());
    auto upperBlockTerm
        = odb::dbBTerm::create(upperBlockNet, interconnectName.c_str());

    lowerBlockTerm->getITerm()->connect(topHierNet);
    upperBlockTerm->getITerm()->connect(topHierNet);

    odb::dbBoolProperty::create(topHierNet, "intersected", true);
    odb::dbBoolProperty::create(lowerBlockNet, "intersected", true);
    odb::dbBoolProperty::create(upperBlockNet, "intersected", true);
    interconnectionNum++;
  }
  logger_->info(
      utl::MDM, 12, "The interconnection number: {}", interconnectionNum);
}

void MultiDieManager::setInterconnectCoordinates()
{
  // set the interconnection (hybrid bond) coordinates
  // check it is multi-die structure
  if (db_->getChip()->getBlock()->getChildren().size() <= 1) {
    logger_->warn(utl::MDM, 14, "This is not multi-die structure");
    return;
  }

  // At the top die, there will be only the interconnections and IO pads
  // if it is multi-die structure.
  auto topBlock = db_->getChip()->getBlock();
  for (auto intersectedTopHierNet : topBlock->getNets()) {
    if (!odb::dbBoolProperty::find(intersectedTopHierNet, "intersected")) {
      continue;
    }
    // Only the intersected nets in lower hierarchy block
    // will be collected in `intersectedNets`.
    vector<odb::dbNet*> intersectedNets;
    // Interconnections as seen from the top hierarchy block
    vector<odb::dbITerm*> interconnectionITerms;
    // Interconnections as seen from the lower hierarchy block
    vector<odb::dbBTerm*> interconnectionBTerms;
    // The size of these vector should be two.

    for (auto iterm : intersectedTopHierNet->getITerms()) {
      if (!iterm->getBTerm()) {
        continue;
      }
      interconnectionITerms.push_back(iterm);
      interconnectionBTerms.push_back(iterm->getBTerm());
      intersectedNets.push_back(iterm->getBTerm()->getNet());
    }
    odb::Rect box1, box2, box;
    box1 = intersectedNets.at(0)->getTermBBox();
    box2 = intersectedNets.at(1)->getTermBBox();
    if (box.intersects(box2)) {
      box = box1.intersect(box2);
    } else {
      vector<int> xCandidates
          = {box1.xMin(), box1.xMax(), box2.xMin(), box2.xMax()};
      vector<int> yCandidates
          = {box1.yMin(), box1.yMax(), box2.yMin(), box2.yMax()};

      // sort by the value
      std::sort(xCandidates.begin(), xCandidates.end());
      std::sort(yCandidates.begin(), yCandidates.end());

      box.init(xCandidates.at(1),
               yCandidates.at(1),
               xCandidates.at(2),
               yCandidates.at(2));
    }
    odb::Point point{box.xCenter(), box.yCenter()};

    // apply the coordinate
    for (auto iTerm : interconnectionITerms) {
      if (!iTerm->getMTerm()->getMPins().empty()) {
        for (auto mPin : iTerm->getMTerm()->getMPins()) {
          // TODO: delete the previous mPin
        }
        // TODO: set the coordinate for the mPin
      }
    }

    for (auto bTerm : interconnectionBTerms) {
      if (!bTerm->getBPins().empty()) {
        for (auto bPin : bTerm->getBPins()) {
          odb::dbBPin::destroy(bPin);
        }
      }
      auto bPin = odb::dbBPin::create(bTerm);
      // todo: layer assignment
      auto topLayerIdx = bTerm->getBlock()->getTech()->getLayerCount();
      auto topLayer = bTerm->getBlock()->getTech()->findLayer(topLayerIdx);
      odb::dbBox::create(bPin,
                         topLayer,
                         point.getX(),
                         point.getY(),
                         point.getX() + 1,
                         point.getY() + 1);
    }
  }
}

}  // namespace mdm
