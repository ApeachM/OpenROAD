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

// clang-format off
#include <fstream>
#include "mdm/MultiDieManager.hh"
#include "TestCaseManager.h"
// clang-format on

namespace mdm {
using namespace std;
using namespace odb;

void MultiDieManager::ICCADParse(const std::string& testCase, bool siteDefined)
{
  testCaseManager_->setSiteDefined(siteDefined);
  // clang-format off
  static const std::unordered_map<std::string, std::function<void()>> testCaseMap {
      {"2022-test1", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE1, this); }},
      {"2022-test2", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE2, this); }},
      {"2022-test2-h", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE2_H, this); }},
      {"2022-test3", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE3, this); }},
      {"2022-test3-h", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE3_H, this); }},
      {"2022-test4", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE4, this); }},
      {"2022-test4-h", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE4_H, this); }},
      {"2023-test1", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE1, this); }},
      {"2023-test2", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE2, this); }},
      {"2023-test2-h", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE2_H, this); }},
      {"2023-test3", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE3, this); }},
      {"2023-test3-h", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE3_H, this); }},
      {"2023-test4", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE4, this); }},
      {"2023-test4-h", [this]() { testCaseManager_->ICCADContest(TestCaseManager::ICCAD2022_CASE4_H, this); }}
  };
  // clang-format on

  auto it = testCaseMap.find(testCase);
  if (it != testCaseMap.end()) {
    it->second();
    logger_->info(
        utl::MDM, 10, "Successfully parsed the test case: {}", testCase);
  } else {
    logger_->error(utl::MDM, 9, "Cannot parse the test case: {}", testCase);
  }
}

void TestCaseManager::ICCADContest(TESTCASE testCase,
                                   MultiDieManager* mdManager)
{
  int year;
  string inputFileName;
  tie(inputFileName, year) = fetchInputFileInfo(testCase);

  if (year == 2022) {
    ICCADContest2022(inputFileName, mdManager);
  } else if (year == 2023) {
    ICCADContest2023(inputFileName, mdManager);
  } else {
    assert(true);
  }

  constructDB(mdManager);
  isICCADParsed_ = true;
}

void TestCaseManager::constructDB(MultiDieManager* mdManager)
{
  auto db_ = mdManager->db_;
  dbTech* dbTechTopHier = dbTech::create(db_, "TopHierTech");
  dbTech* dbTechTop = dbTech::create(db_, "Die1Tech");
  dbTech* dbTechBottom = dbTech::create(db_, "Die2Tech");
  dbTechLayer* dbTechLayerTopHier = dbTechLayer::create(
      dbTechTopHier, "layer", dbTechLayerType::MASTERSLICE);
  dbTechLayer* dbTechLayerTop
      = dbTechLayer::create(dbTechTop, "layer", dbTechLayerType::MASTERSLICE);
  dbTechLayer* dbTechLayerBottom = dbTechLayer::create(
      dbTechBottom, "layer", dbTechLayerType::MASTERSLICE);

  // make M2 layer
  // the width and spacing is arbitrarily decided.
  dbTechLayer* dbTechLayerTopHierM2
      = dbTechLayer::create(dbTechTopHier, "M2", dbTechLayerType::ROUTING);
  dbTechLayerTopHierM2->setWidth(10);
  dbTechLayerTopHierM2->setSpacing(10);
  dbTechLayerTopHierM2->setDirection(dbTechLayerDir::HORIZONTAL);
  dbTechLayer* dbTechLayerTopM2
      = dbTechLayer::create(dbTechTop, "M2", dbTechLayerType::ROUTING);
  dbTechLayerTopM2->setWidth(10);
  dbTechLayerTopM2->setSpacing(10);
  dbTechLayerTopM2->setDirection(dbTechLayerDir::HORIZONTAL);
  dbTechLayer* dbTechLayerBottomM2
      = dbTechLayer::create(dbTechBottom, "M2", dbTechLayerType::ROUTING);
  dbTechLayerBottomM2->setWidth(10);
  dbTechLayerBottomM2->setSpacing(10);
  dbTechLayerBottomM2->setDirection(dbTechLayerDir::HORIZONTAL);

  dbLib* dbLibTopHier = dbLib::create(db_, "TopHierLib", dbTechTopHier);
  dbLib* dbLibTop = dbLib::create(db_, "TopLib", dbTechTop);
  dbLib* dbLibBottom = dbLib::create(db_, "BottomLib", dbTechBottom);
  dbChip* dbChip = dbChip::create(db_);
  dbBlock* dbBlockTopHier
      = dbBlock::create(dbChip, "topHierBlock", dbTechTopHier);

  DieInfo* topDieInfo = &benchInformation_.dieInfos.at(0);
  DieInfo* bottomDieInfo = &benchInformation_.dieInfos.at(1);

  // Die Area Construction //
  int lowerLeftX, lowerLeftY, upperRightX, upperRightY;
  // the size of top die and the one of bottom die are same each other.
  lowerLeftX = topDieInfo->lowerLeftX * scale_;
  lowerLeftY = topDieInfo->lowerLeftY * scale_;
  upperRightX = topDieInfo->upperRightX * scale_;
  upperRightY = topDieInfo->upperRightY * scale_;
  Point lowerLeft = Point(lowerLeftX, lowerLeftY);
  Point upperRight = Point(upperRightX, upperRightY);
  Rect dieArea(lowerLeft, upperRight);
  db_->getChip()->getBlock()->setDieArea(dieArea);

  // Row construction

  // Top hier and top die
  dbSite* site = dbSite::create(dbLibTopHier, "Site");
  uint site_width = scale_;
  int siteHeight = rowInfos_.first.rowHeight * scale_;

  site->setWidth(site_width);
  site->setHeight(siteHeight);

  int numOfSites = floor(rowInfos_.first.rowWidth * scale_ / site_width);
  int numOfRow = rowInfos_.first.repeatCount;
  while (numOfRow * siteHeight > topDieInfo->upperRightY * scale_) {
    numOfRow--;
  }
  for (int i = 0; i < numOfRow; ++i) {
    dbRow::create(dbBlockTopHier,
                  ("row" + to_string(i)).c_str(),
                  site,
                  0,
                  i * siteHeight,
                  dbOrientType::MX,
                  dbRowDir::HORIZONTAL,
                  numOfSites,
                  site_width);
  }

  // LibCell Construction //
  assert(topDieInfo->techInfo->libCellNum
         == bottomDieInfo->techInfo->libCellNum);
  int libCellNum = topDieInfo->techInfo->libCellNum;
  for (int i = 0; i < libCellNum; ++i) {
    LibCellInfo* libCellInfoTop = &topDieInfo->techInfo->libCellInfos.at(i);
    LibCellInfo* libCellInfoBottom
        = &bottomDieInfo->techInfo->libCellInfos.at(i);
    string libCellNameTop = libCellInfoTop->name;
    string libCellNameBottom = libCellInfoBottom->name;
    assert(libCellInfoTop->name == libCellInfoBottom->name);
    int widthTop = libCellInfoTop->width * scale_;
    int widthBottom = libCellInfoBottom->width * scale_;
    int heightTop = libCellInfoTop->height * scale_;
    int heightBottom = libCellInfoBottom->height * scale_;
    int pinNumTop = libCellInfoTop->pinNumber;
    int pinNumBottom = libCellInfoBottom->pinNumber;
    assert(pinNumTop == pinNumBottom);
    int pinNum = pinNumTop;

    dbMaster* dbMasterTopHier
        = dbMaster::create(dbLibTopHier, (libCellNameTop).c_str());
    dbMaster* dbMasterTop
        = dbMaster::create(dbLibTop, (libCellNameTop).c_str());
    dbMaster* dbMasterBottom
        = dbMaster::create(dbLibBottom, (libCellNameBottom).c_str());

    dbMasterTopHier->setWidth(widthTop);
    dbMasterTopHier->setHeight(heightTop);
    dbMasterTop->setWidth(widthTop);
    dbMasterTop->setHeight(heightTop);
    dbMasterBottom->setWidth(widthBottom);
    dbMasterBottom->setHeight(heightBottom);

    if (libCellInfoTop->isMacro) {
      dbMasterTop->setType(dbMasterType::BLOCK);
      dbMasterTopHier->setType(dbMasterType::BLOCK);
    } else {
      dbMasterTop->setType(dbMasterType::CORE);
      dbMasterTopHier->setType(dbMasterType::CORE);
      dbMasterTopHier->setSite(site);
    }
    if (libCellInfoBottom->isMacro) {
      dbMasterBottom->setType(dbMasterType::BLOCK);
    } else {
      dbMasterBottom->setType(dbMasterType::CORE);
    }

    // read pins in each lib cell //
    for (int j = 0; j < pinNum; ++j) {
      LibPinInfo* pinInfoTop = &libCellInfoTop->libPinInfos.at(j);
      LibPinInfo* pinInfoBottom = &libCellInfoBottom->libPinInfos.at(j);
      string pinNameTop = pinInfoTop->pinName;
      string pinNameBottom = pinInfoBottom->pinName;
      assert(pinNameTop == pinNameBottom);
      int pinLocationXTop = pinInfoTop->pinLocationX * scale_;
      int pinLocationYTop = pinInfoTop->pinLocationY * scale_;
      int pinLocationXBottom = pinInfoBottom->pinLocationX * scale_;
      int pinLocationYBottom = pinInfoBottom->pinLocationY * scale_;

      // There's no information in this contest benchmarks.
      // assume that the cell has only one output port.
      dbIoType io_type;
      if (j != pinNum - 1) {
        io_type = dbIoType::INPUT;
      } else {
        io_type = dbIoType::OUTPUT;
      }
      dbSigType sig_type = dbSigType::SIGNAL;  // There's no information in this
                                               // contest benchmarks.
      dbMTerm* master_terminal_top_hier = dbMTerm::create(
          dbMasterTopHier, pinNameTop.c_str(), io_type, sig_type);
      dbMTerm* master_terminal_top
          = dbMTerm::create(dbMasterTop, pinNameTop.c_str(), io_type, sig_type);
      dbMTerm* master_terminal_bottom = dbMTerm::create(
          dbMasterBottom, pinNameBottom.c_str(), io_type, sig_type);
      dbMPin* master_pin_top_hier = dbMPin::create(master_terminal_top_hier);
      dbMPin* master_pin_top = dbMPin::create(master_terminal_top);
      dbMPin* master_pin_bottom = dbMPin::create(master_terminal_bottom);
      dbBox::create(master_pin_top_hier,
                    dbTechLayerTopHier,
                    pinLocationXTop,
                    pinLocationYTop,
                    pinLocationXTop + 1 * scale_,
                    pinLocationYTop + 1 * scale_);
      dbBox::create(master_pin_top,
                    dbTechLayerTop,
                    pinLocationXTop,
                    pinLocationYTop,
                    pinLocationXTop + 1 * scale_,
                    pinLocationYTop + 1 * scale_);
      dbBox::create(master_pin_bottom,
                    dbTechLayerBottom,
                    pinLocationXBottom,
                    pinLocationYBottom,
                    pinLocationXBottom + 1 * scale_,
                    pinLocationYBottom + 1 * scale_);
    }
    dbMasterTopHier->setFrozen();
    dbMasterTop->setFrozen();
    dbMasterBottom->setFrozen();
  }

  // Instance Construction //
  // Before partition, let the all the instance is on the bottom.
  string instance_name;
  string master_name;

  for (const auto& instance_info : benchInformation_.instance_infos) {
    instance_name = instance_info.instName;
    master_name = instance_info.libCellName;
    dbMaster* master
        = db_->findLib("TopHierLib")->findMaster(master_name.c_str());
    assert(master != nullptr);
    dbInst::create(dbBlockTopHier, master, instance_name.c_str());
  }

  // Net Construction //
  for (auto& net_info : benchInformation_.netInfos) {
    dbNet* net = dbNet::create(dbBlockTopHier, net_info.netName.c_str());

    // read pins in one Net
    for (int j = 0; j < net_info.pinNum; ++j) {
      ConnectedPinInfo* pin_info = &net_info.connectedPinsInfos.at(j);
      dbInst* inst = dbBlockTopHier->findInst((pin_info->instanceName).c_str());
      assert(inst != nullptr);
      assert(inst->findITerm(pin_info->libPinName.c_str()));
      inst->findITerm(pin_info->libPinName.c_str())->connect(net);
    }
  }

  // hybrid bond info
  int x = benchInformation_.terminalInfo.sizeX * scale_;
  int y = benchInformation_.terminalInfo.sizeY * scale_;
  int space = benchInformation_.terminalInfo.spacing_size * scale_;
  int cost = benchInformation_.terminalInfo.cost;
  odb::dbIntProperty::create(db_->getChip(), "hybridBondX", x);
  odb::dbIntProperty::create(db_->getChip(), "hybridBondY", y);
  odb::dbIntProperty::create(db_->getChip(), "hybridBondSpacing", space);
  odb::dbIntProperty::create(db_->getChip(), "hybridBondCost", cost);
}

void TestCaseManager::rowConstruction()
{
  auto libIter = mdm_->db_->getLibs().begin();
  auto blockIter = mdm_->db_->getChip()->getBlock()->getChildren().begin();
  std::advance(libIter, 1);

  for (int i = 0; i < 2; ++i) {
    auto lib = *libIter;
    auto block = *blockIter;

    dbSite* site;
    uint site_width;
    int siteHeight, numOfSites, numOfRow;
    if (i == 0) {
      site = dbSite::create(lib, "TopSite");
      site_width = getSiteWidth().first * scale_;
      siteHeight = rowInfos_.first.rowHeight * scale_;
      numOfSites = floor(rowInfos_.first.rowWidth * scale_ / site_width);
      numOfRow = rowInfos_.first.repeatCount;
    } else {
      site = dbSite::create(lib, "BottomSite");
      site_width = getSiteWidth().second * scale_;
      siteHeight = rowInfos_.second.rowHeight * scale_;
      numOfSites = floor(rowInfos_.second.rowWidth * scale_ / site_width);
      numOfRow = rowInfos_.second.repeatCount;
    }

    site->setWidth(site_width);
    site->setHeight(siteHeight);

    for (int j = 0; j < numOfRow; ++j) {
      dbRow::create(block,
                    ("row" + to_string(j)).c_str(),
                    site,
                    0,
                    j * siteHeight,
                    dbOrientType::MX,
                    dbRowDir::HORIZONTAL,
                    numOfSites,
                    site_width);
    }

    std::advance(libIter, 1);
    std::advance(blockIter, 1);
  }
}

void TestCaseManager::ICCADContest2022(const string& inputFileName,
                                       MultiDieManager* mdManager)
{
  ifstream input_file(inputFileName);
  if (!input_file.is_open()) {
    mdManager->logger_->error(
        utl::MDM, 8, "Cannot open the input file: {}", inputFileName);
  }

  std::string info, name1, name2;
  int n1, n2, n3, n4, n5;

  // Syntax: NumTechnologies <technologyCount>
  input_file >> info >> n1;
  assert(info == "NumTechnologies");
  mdManager->numberOfDie_ = n1;

  for (int i = 0; i < mdManager->numberOfDie_; ++i) {
    // Syntax: Tech <techName> <libCellCount>
    input_file >> info >> name1 >> n1;
    assert(info == "Tech");

    TechInfo tech_info;
    tech_info.name = name1;
    tech_info.libCellNum = n1;

    // read LibCells in one tech
    for (int j = 0; j < tech_info.libCellNum; ++j) {
      // Syntax: LibCell <libCellName> <libCellSizeX> <libCellSizeY>
      // <pinCount>
      input_file >> info >> name1 >> n1 >> n2 >> n3;
      assert(info == "LibCell");

      LibCellInfo lib_cell_info;
      lib_cell_info.isMacro = false;
      lib_cell_info.name = name1;
      lib_cell_info.width = n1;
      lib_cell_info.height = n2;
      lib_cell_info.pinNumber = n3;

      for (int k = 0; k < lib_cell_info.pinNumber; ++k) {
        // Syntax: Pin <pinName> <pinLocationX> <pinLocationY>
        input_file >> info >> name1 >> n1 >> n2;
        LibPinInfo lib_pin_info;
        lib_pin_info.pinName = name1;
        lib_pin_info.pinLocationX = n1;
        lib_pin_info.pinLocationY = n2;
        lib_cell_info.libPinInfos.push_back(lib_pin_info);
      }
      tech_info.libCellInfos.push_back(lib_cell_info);
    }
    benchInformation_.techInfos.push_back(tech_info);
  }

  // Syntax: DieSize <lowerLeftX> <lowerLeftY> <upperRightX> <upperRightY>
  input_file >> info >> n1 >> n2 >> n3 >> n4;
  assert(info == "DieSize");

  for (int i = 0; i < 2; ++i) {
    DieInfo die_info;
    die_info.lowerLeftX = n1;
    die_info.lowerLeftY = n2;
    die_info.upperRightX = n3;
    die_info.upperRightY = n4;
    benchInformation_.dieInfos.push_back(die_info);
  }

  // Syntax: TopDieMaxUtil <util>
  input_file >> info >> n1;
  assert(info == "TopDieMaxUtil");
  benchInformation_.dieInfos.at(TOP_DIE).max_util = n1;

  // Syntax: BottomDieMaxUtil <util>
  input_file >> info >> n1;
  assert(info == "BottomDieMaxUtil");
  benchInformation_.dieInfos.at(BOTTOM_DIE).max_util = n1;

  // Syntax: TopDieRows <startX> <startY> <rowLength> <rowHeight> <repeatCount>
  input_file >> info >> n1 >> n2 >> n3 >> n4 >> n5;
  assert(info == "TopDieRows");
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.startX = n1;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.startY = n2;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.rowWidth = n3;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.rowHeight = n4;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.repeatCount = n5;

  // Syntax: BottomDieRows <startX> <startY> <rowLength> <rowHeight>
  // <repeatCount>
  input_file >> info >> n1 >> n2 >> n3 >> n4 >> n5;
  assert(info == "BottomDieRows");
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.startX = n1;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.startY = n2;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.rowWidth = n3;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.rowHeight = n4;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.repeatCount = n5;

  // Syntax: TopDieTech <TechName>
  input_file >> info >> name1;
  assert(info == "TopDieTech");
  benchInformation_.dieInfos.at(TOP_DIE).tech_name = name1;

  // Syntax: BottomDieTech <TechName>
  input_file >> info >> name1;
  assert(info == "BottomDieTech");
  benchInformation_.dieInfos.at(BOTTOM_DIE).tech_name = name1;

  // Syntax: TerminalSize <sizeX> <sizeY>
  input_file >> info >> n1 >> n2;
  assert(info == "TerminalSize");
  benchInformation_.terminalInfo.sizeX = n1;
  benchInformation_.terminalInfo.sizeY = n2;

  // Syntax: TerminalSpacing <spacing>
  input_file >> info >> n1;
  assert(info == "TerminalSpacing");
  benchInformation_.terminalInfo.spacing_size = n1;

  benchInformation_.terminalInfo.cost = 0;

  // Syntax: NumInstances <instanceCount>
  input_file >> info >> n1;
  assert(info == "NumInstances");
  instanceNumber_ = n1;

  // read instances
  for (int i = 0; i < instanceNumber_; ++i) {
    // Syntax: Inst <instName> <libCellName>
    input_file >> info >> name1 >> name2;
    assert(info == "Inst");
    InstanceInfo instance_info;
    instance_info.instName = name1;
    instance_info.libCellName = name2;
    benchInformation_.instance_infos.push_back(instance_info);
  }

  // Syntax: NumNets <netCount>
  input_file >> info >> n1;
  assert(info == "NumNets");
  netNumber_ = n1;

  for (int i = 0; i < netNumber_; ++i) {
    // Syntax: Net <netName> <numPins>
    input_file >> info >> name1 >> n1;
    assert(info == "Net");
    NetInfo net_info;
    net_info.netName = name1;
    net_info.pinNum = n1;

    // read pins in one Net
    for (int j = 0; j < net_info.pinNum; ++j) {
      // Syntax: Pin <instName>/<libPinName>
      input_file >> info >> name1;
      assert(info == "Pin");

      int idx = name1.find('/');  // NOLINT(*-narrowing-conversions)
      string inst_name = name1.substr(0, idx);
      string lib_pin_name = name1.substr(idx + 1);
      ConnectedPinInfo pin_info;
      pin_info.instanceName = inst_name;
      pin_info.libPinName = lib_pin_name;
      net_info.connectedPinsInfos.push_back(pin_info);
    }
    benchInformation_.netInfos.push_back(net_info);
  }

  for (DieInfo& die_info : benchInformation_.dieInfos) {
    for (TechInfo& tech_info : benchInformation_.techInfos) {
      if (die_info.tech_name == tech_info.name) {
        die_info.techInfo = &tech_info;
      }
    }
  }

  rowInfos_.first = benchInformation_.dieInfos.at(TOP_DIE).rowInfo;
  maxUtils_.first = benchInformation_.dieInfos.at(TOP_DIE).max_util;
  rowInfos_.second = benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo;
  maxUtils_.second = benchInformation_.dieInfos.at(BOTTOM_DIE).max_util;

  input_file.close();
}

void TestCaseManager::ICCADContest2023(const string& inputFileName,
                                       MultiDieManager* mdManager)
{
  ifstream input_file(inputFileName);
  if (!input_file.is_open()) {
    mdManager->logger_->error(
        utl::MDM, 7, "Cannot open the input file: {}", inputFileName);
  }

  std::string info, name1, name2;
  int n1, n2, n3, n4, n5;

  // Syntax: NumTechnologies <technologyCount>
  input_file >> info >> n1;
  assert(info == "NumTechnologies");
  mdManager->numberOfDie_ = n1;

  for (int i = 0; i < mdManager->numberOfDie_; ++i) {
    // Syntax: Tech <techName> <libCellCount>
    input_file >> info >> name1 >> n1;
    assert(info == "Tech");

    TechInfo tech_info;
    tech_info.name = name1;
    tech_info.libCellNum = n1;

    // read LibCells in one tech
    for (int j = 0; j < tech_info.libCellNum; ++j) {
      // Syntax: LibCell <isMacro> <libCellName> <libCellSizeX> <libCellSizeY>
      // <pinCount>
      input_file >> info >> name1 >> name2 >> n1 >> n2 >> n3;
      assert(info == "LibCell");

      LibCellInfo lib_cell_info;
      if (name1 == "N") {
        lib_cell_info.isMacro = false;
      } else if (name1 == "Y") {
        lib_cell_info.isMacro = true;
      } else {
        assert(false);
      }
      lib_cell_info.name = name2;
      lib_cell_info.width = n1;
      lib_cell_info.height = n2;
      lib_cell_info.pinNumber = n3;

      for (int k = 0; k < lib_cell_info.pinNumber; ++k) {
        // Syntax: Pin <pinName> <pinLocationX> <pinLocationY>
        input_file >> info >> name1 >> n1 >> n2;
        LibPinInfo lib_pin_info;
        lib_pin_info.pinName = name1;
        lib_pin_info.pinLocationX = n1;
        lib_pin_info.pinLocationY = n2;
        lib_cell_info.libPinInfos.push_back(lib_pin_info);
      }
      tech_info.libCellInfos.push_back(lib_cell_info);
    }
    benchInformation_.techInfos.push_back(tech_info);
  }

  // Syntax: DieSize <lowerLeftX> <lowerLeftY> <upperRightX> <upperRightY>
  input_file >> info >> n1 >> n2 >> n3 >> n4;
  assert(info == "DieSize");

  for (int i = 0; i < 2; ++i) {
    DieInfo die_info;
    die_info.lowerLeftX = n1;
    die_info.lowerLeftY = n2;
    die_info.upperRightX = n3;
    die_info.upperRightY = n4;
    benchInformation_.dieInfos.push_back(die_info);
  }

  // Syntax: TopDieMaxUtil <util>
  input_file >> info >> n1;
  assert(info == "TopDieMaxUtil");
  benchInformation_.dieInfos.at(TOP_DIE).max_util = n1;

  // Syntax: BottomDieMaxUtil <util>
  input_file >> info >> n1;
  assert(info == "BottomDieMaxUtil");
  benchInformation_.dieInfos.at(BOTTOM_DIE).max_util = n1;

  // Syntax: TopDieRows <startX> <startY> <rowLength> <rowHeight> <repeatCount>
  input_file >> info >> n1 >> n2 >> n3 >> n4 >> n5;
  assert(info == "TopDieRows");
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.startX = n1;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.startY = n2;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.rowWidth = n3;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.rowHeight = n4;
  benchInformation_.dieInfos.at(TOP_DIE).rowInfo.repeatCount = n5;

  // Syntax: BottomDieRows <startX> <startY> <rowLength> <rowHeight>
  // <repeatCount>
  input_file >> info >> n1 >> n2 >> n3 >> n4 >> n5;
  assert(info == "BottomDieRows");
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.startX = n1;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.startY = n2;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.rowWidth = n3;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.rowHeight = n4;
  benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo.repeatCount = n5;

  // Syntax: TopDieTech <TechName>
  input_file >> info >> name1;
  assert(info == "TopDieTech");
  benchInformation_.dieInfos.at(TOP_DIE).tech_name = name1;

  // Syntax: BottomDieTech <TechName>
  input_file >> info >> name1;
  assert(info == "BottomDieTech");
  benchInformation_.dieInfos.at(BOTTOM_DIE).tech_name = name1;

  // Syntax: TerminalSize <sizeX> <sizeY>
  input_file >> info >> n1 >> n2;
  assert(info == "TerminalSize");
  benchInformation_.terminalInfo.sizeX = n1;
  benchInformation_.terminalInfo.sizeY = n2;

  // Syntax: TerminalSpacing <spacing>
  input_file >> info >> n1;
  assert(info == "TerminalSpacing");
  benchInformation_.terminalInfo.spacing_size = n1;

  // Syntax: TerminalCost <terminalCost>
  input_file >> info >> n1;
  assert(info == "TerminalCost");
  benchInformation_.terminalInfo.cost = n1;

  // Syntax: NumInstances <instanceCount>
  input_file >> info >> n1;
  assert(info == "NumInstances");
  instanceNumber_ = n1;

  // read instances
  for (int i = 0; i < instanceNumber_; ++i) {
    // Syntax: Inst <instName> <libCellName>
    input_file >> info >> name1 >> name2;
    assert(info == "Inst");
    InstanceInfo instance_info;
    instance_info.instName = name1;
    instance_info.libCellName = name2;
    benchInformation_.instance_infos.push_back(instance_info);
  }

  // Syntax: NumNets <netCount>
  input_file >> info >> n1;
  assert(info == "NumNets");
  netNumber_ = n1;

  for (int i = 0; i < netNumber_; ++i) {
    // Syntax: Net <netName> <numPins>
    input_file >> info >> name1 >> n1;
    assert(info == "Net");
    NetInfo net_info;
    net_info.netName = name1;
    net_info.pinNum = n1;

    // read pins in one Net
    for (int j = 0; j < net_info.pinNum; ++j) {
      // Syntax: Pin <instName>/<libPinName>
      input_file >> info >> name1;
      assert(info == "Pin");

      int idx = name1.find('/');  // NOLINT(*-narrowing-conversions)
      string inst_name = name1.substr(0, idx);
      string lib_pin_name = name1.substr(idx + 1);
      ConnectedPinInfo pin_info;
      pin_info.instanceName = inst_name;
      pin_info.libPinName = lib_pin_name;
      net_info.connectedPinsInfos.push_back(pin_info);
    }
    benchInformation_.netInfos.push_back(net_info);
  }

  for (DieInfo& die_info : benchInformation_.dieInfos) {
    for (TechInfo& tech_info : benchInformation_.techInfos) {
      if (die_info.tech_name == tech_info.name) {
        die_info.techInfo = &tech_info;
      }
    }
  }

  rowInfos_.first = benchInformation_.dieInfos.at(TOP_DIE).rowInfo;
  maxUtils_.first = benchInformation_.dieInfos.at(TOP_DIE).max_util;
  rowInfos_.second = benchInformation_.dieInfos.at(BOTTOM_DIE).rowInfo;
  maxUtils_.second = benchInformation_.dieInfos.at(BOTTOM_DIE).max_util;

  input_file.close();
}

pair<string, int> TestCaseManager::fetchInputFileInfo(TESTCASE testCase)
{
  std::map<TESTCASE, std::pair<std::string, int>> testCaseFileName;
  // clang-format off
  if (!siteDefined_) {
    testCaseFileName[ICCAD2022_CASE1] = {"ICCAD/2022/case1.txt", 2022};
    testCaseFileName[ICCAD2022_CASE2] = {"ICCAD/2022/case2.txt", 2022};
    testCaseFileName[ICCAD2022_CASE3] = {"ICCAD/2022/case3.txt", 2022};
    testCaseFileName[ICCAD2022_CASE4] = {"ICCAD/2022/case4.txt", 2022};
    testCaseFileName[ICCAD2022_CASE2_H] = {"ICCAD/2022/case2_h.txt", 2022};
    testCaseFileName[ICCAD2022_CASE3_H] = {"ICCAD/2022/case3_h.txt", 2022};
    testCaseFileName[ICCAD2022_CASE4_H] = {"ICCAD/2022/case4_h.txt", 2022};
    testCaseFileName[ICCAD2023_CASE1] = {"ICCAD/2023/case1.txt", 2023};
    testCaseFileName[ICCAD2023_CASE2] = {"ICCAD/2023/case2.txt", 2023};
    testCaseFileName[ICCAD2023_CASE3] = {"ICCAD/2023/case3.txt", 2023};
    testCaseFileName[ICCAD2023_CASE4] = {"ICCAD/2023/case4.txt", 2023};
    testCaseFileName[ICCAD2023_CASE2_H] = {"ICCAD/2023/case2_h.txt", 2023};
    testCaseFileName[ICCAD2023_CASE3_H] = {"ICCAD/2023/case3_h.txt", 2023};
    testCaseFileName[ICCAD2023_CASE4_H] = {"ICCAD/2023/case4_h.txt", 2023};
  } else {
    testCaseFileName[ICCAD2022_CASE1] = {"ICCAD/2022/siteDefined_case1.txt", 2022};
    testCaseFileName[ICCAD2022_CASE2] = {"ICCAD/2022/siteDefined_case2.txt", 2022};
    testCaseFileName[ICCAD2022_CASE3] = {"ICCAD/2022/siteDefined_case3.txt", 2022};
    testCaseFileName[ICCAD2022_CASE4] = {"ICCAD/2022/siteDefined_case4.txt", 2022};
    testCaseFileName[ICCAD2022_CASE2_H] = {"ICCAD/2022/siteDefined_case2_h.txt", 2022};
    testCaseFileName[ICCAD2022_CASE3_H] = {"ICCAD/2022/siteDefined_case3_h.txt", 2022};
    testCaseFileName[ICCAD2022_CASE4_H] = {"ICCAD/2022/siteDefined_case4_h.txt", 2022};
    testCaseFileName[ICCAD2023_CASE1] = {"ICCAD/2023/siteDefined_case1.txt", 2023};
    testCaseFileName[ICCAD2023_CASE2] = {"ICCAD/2023/siteDefined_case2.txt", 2023};
    testCaseFileName[ICCAD2023_CASE3] = {"ICCAD/2023/siteDefined_case3.txt", 2023};
    testCaseFileName[ICCAD2023_CASE4] = {"ICCAD/2023/siteDefined_case4.txt", 2023};
    testCaseFileName[ICCAD2023_CASE2_H] = {"ICCAD/2023/siteDefined_case2_h.txt", 2023};
    testCaseFileName[ICCAD2023_CASE3_H] = {"ICCAD/2023/siteDefined_case3_h.txt", 2023};
    testCaseFileName[ICCAD2023_CASE4_H] = {"ICCAD/2023/siteDefined_case4_h.txt", 2023};
  }
  // clang-format on
  testcase_ = testCase;
  auto it = testCaseFileName.find(testCase);
  if (it != testCaseFileName.end()) {
    return it->second;
  }
  assert(false);
}

std::pair<int, int> TestCaseManager::getSiteWidth()
{
  map<TESTCASE, vector<int>> data;
  data[ICCAD2022_CASE2] = {21, 30};
  data[ICCAD2022_CASE2_H] = {30};
  data[ICCAD2022_CASE3] = {14};
  data[ICCAD2022_CASE3_H] = {11, 14};
  data[ICCAD2022_CASE4] = {17, 21};
  data[ICCAD2022_CASE4_H] = {18, 21};

  int widthTA = 1;
  int widthTB = 1;
  if (siteDefined_) {
    auto it = data.find(testcase_);
    if (it != data.end()) {
      widthTA = it->second.size() > 0 ? it->second.at(0) : 1;
      widthTB = it->second.size() > 1 ? it->second.at(1) : widthTA;
    }
  }

  widthTA *= scale_;
  widthTB *= scale_;
  return std::make_pair(widthTA, widthTB);
}

void TestCaseManager::setScale(int scale)
{
  scale_ = scale;
  iccadOutputParser_.setScale(scale);
}

void TestCaseManager::parseICCADOutput(const std::string& fileName,
                                       const char* whichDieChar)
{
  ifstream inputFile(fileName);
  string whichDie{whichDieChar};
  if (!inputFile.is_open()) {
    mdm_->logger_->warn(
        utl::MDM, 11, "Cannot open the input file: {}", fileName);
  }
  iccadOutputParser_.parseOutput(inputFile);
  if (whichDie == "top") {
    iccadOutputParser_.applyCoordinates(
        *mdm_->db_->getChip()->getBlock()->getChildren().begin());
  } else if (whichDie == "bottom") {
    auto it = mdm_->db_->getChip()->getBlock()->getChildren().begin();
    std::advance(it, 1);
    iccadOutputParser_.applyCoordinates(*it);
  } else {
    iccadOutputParser_.applyCoordinates();
  }

  iccadOutputParser_.makePartitionFile(fileName + ".par");
  inputFile.close();
}

void TestCaseManager::setMDM(MultiDieManager* mdm)
{
  mdm_ = mdm;
  iccadOutputParser_.setDb(mdm_->getDB());
}

void TestCaseManager::setSiteDefined(bool siteDefined)
{
  siteDefined_ = siteDefined;
}

void ICCADOutputParser::parseOutput(ifstream& outputFile)
{
  string info, name;
  int n1;
  // TopDiePlacement <instance #>
  outputFile >> info >> n1;
  assert(info == "TopDiePlacement");
  for (int i = 0; i < n1; ++i) {
    int x, y;
    // Inst <instance name> <instance coordinate: X, Y>
    outputFile >> info >> name >> x >> y;
    assert(info == "Inst");
    instList_.push_back(name);
    instPartitionMap_[name] = TOP;
    instCoordinateMap_[name] = {x, y};
  }

  // BottomDiePlacement <instance #>
  outputFile >> info >> n1;
  assert(info == "BottomDiePlacement");
  for (int i = 0; i < n1; ++i) {
    int x, y;
    // Inst <instance name> <instance coordinate: X, Y>
    outputFile >> info >> name >> x >> y;
    assert(info == "Inst");
    instList_.push_back(name);
    instPartitionMap_[name] = BOTTOM;
    instCoordinateMap_[name] = {x, y};
  }

  // NumTerminals <terminal #>
  outputFile >> info >> n1;
  assert(info == "NumTerminals");
  for (int i = 0; i < n1; ++i) {
    int x, y;
    // Inst <instance name> <instance coordinate: X, Y>
    outputFile >> info >> name >> x >> y;
    assert(info == "Terminal");
    terminalList_.push_back(name);
    terminalCoordinateMap_[name] = {x, y};
  }

  parsed_ = true;
}

void ICCADOutputParser::applyCoordinates(odb::dbBlock* targetBlock)
{  // apply the parsed coordinate to the database
  vector<odb::dbBlock*> blockSet;
  if (targetBlock == nullptr) {
    blockSet.push_back(db_->getChip()->getBlock());
    for (auto block : db_->getChip()->getBlock()->getChildren()) {
      blockSet.push_back(block);
    }
  } else {
    blockSet.push_back(targetBlock);
  }
  for (auto block : blockSet) {
    for (auto instance : block->getInsts()) {
      string instName = instance->getName();
      auto coordinate = getCellCoordinate(instName);
      instance->setLocation(coordinate.first * scale_,
                            coordinate.second * scale_);
      instance->setPlacementStatus(dbPlacementStatus::PLACED);
    }
  }
}

void ICCADOutputParser::makePartitionFile(const string& fileName)
{
  if (!parsed_) {
    return;
  }
  std::ofstream outFile(fileName);
  if (outFile.is_open()) {
    for (const auto& instName : instList_) {
      outFile << instName << "  " << instPartitionMap_[instName] << "\n";
    }
  }
}

}  // namespace mdm
