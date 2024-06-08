/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#ifndef OPENROAD_SRC_MDM_INCLUDE_MDM_MULTIDIEMANAGER_H_
#define OPENROAD_SRC_MDM_INCLUDE_MDM_MULTIDIEMANAGER_H_
#include <tcl.h>

#include <iostream>
#include <string>
#include <vector>

namespace odb {
class dbDatabase;
class dbBlock;
class dbTech;
class dbLib;
class dbInst;
}  // namespace odb
namespace sta {
class dbSta;
}
namespace par {
class PartitionMgr;
}
namespace gpl {
class Replace;
}
namespace utl {
class Logger;
}
namespace dpl {
class Opendp;
}

namespace mdm {
class MultiDieManager;
class TestCaseManager;

class SwitchInstanceHelper
{
 public:
  static void switchInstanceToAssignedDie(MultiDieManager* manager,
                                          odb::dbInst* originalInst);

 private:
  static int findAssignedDieId(odb::dbInst* inst);
  static std::pair<odb::dbBlock*, odb::dbLib*> findTargetDieAndLib(
      MultiDieManager* manager,
      int dieID);
  static void inheritPlacementInfo(odb::dbInst* originalInst,
                                   odb::dbInst* newInst);
  static void inheritNetInfo(odb::dbInst* originalInst, odb::dbInst* newInst);
  static void inheritProperties(odb::dbInst* originalInst,
                                odb::dbInst* newInst);
  static void inheritGroupInfo(odb::dbInst* originalInst, odb::dbInst* newInst);
};

class MultiDieManager
{
  friend class SwitchInstanceHelper;
  friend class TestCaseManager;

 public:
  MultiDieManager();
  ~MultiDieManager();

  void init(odb::dbDatabase* db,
            utl::Logger* logger,
            par::PartitionMgr* partitionMgr,
            gpl::Replace* replace,
            dpl::Opendp* opendp,
            sta::dbSta* sta);

  void set3DIC(int numberOfDie, float areaRatio = 0.5);

  odb::dbDatabase* getDB() { return db_; }

  void get3DHPWL(bool approximate = true);
  void getHPWL(const char* dieInfo = nullptr);

  /**
   * \brief
   * Make shrunk lef files.
   * If the area_ratio is 0.5,
   * then the first lef ratio is 1.0, the second lef ratio is 0.5,
   * the third lef ratio is 0.25 .. \n
   * This affects only library information.
   * This will not affect tech information, because the multi several tech is
   * not supported completely in odb.
   * */
  void makeShrunkLefs();

  void multiDieDPL();

  void multiDieDPO();

  void runSemiLegalizer(char* targetDie = "");

  /**
   * \pre
   * `mdm::makeInterconnections` & placement
   * */
  void setInterconnectCoordinates();

  void constructSimpleExample1();

  /**
   * Before calling this function,
   * you need to load the lef in test/timing/example1.lef
   * Refer to the test/mdm-timingTest1.tcl
   * */
  void timingTest1();

  void timingTestOneDie();

  void ICCADParse(const std::string& testCase, bool siteDefined = false);

  void setPartitionFile(char* partitionFile);

  void setICCADScale(int scale);

  /**
   * This is Experiment utilization.
   * \brief
   * Exports instance coordinates for debugging.
   * These are parsed by another function to obtain die coordinates temporarily.
   * */
  void exportCoordinates(char* fileName);

  /**
   * This is Experiment utilization.
   * \brief
   * This is Experiment utilization.
   * Imports and parses exported file to temporarily retrieve die coordinates
   * for debugging.
   * */
  void importCoordinates(char* fileName);

  /**
   * This is Experiment utilization.
   * You need not to call set_3D_IC command before calling this function.
   * \pre readPartitionInfo()
   * \arg TOP || BOTTOM
   * This function will be removed.
   * */
  void destroyOneDie(char* DIE);

  /**
   * The reading function for partitioning information in `par` doesn't
   * work properly. So this function is made as a temporal solution.
   * */
  void readPartitionInfo(const char* fileNameChar);

  void parseICCADOutput(char* filenameChar, char* whichDie = "");

 private:
  odb::dbTech* makeNewTech(const std::string& techName);
  void makeShrunkLib(const std::string& whichDie,
                     double shrunkRatio,
                     odb::dbTech* tech__);

  /**
   * \brief
   * Partition instances into different die.
   * This will consider the size of technologies and the utilization of each
   * die.
   *
   * 1. Read partition information from file.
   * 2. Make sub-blocks as much as partition IDs.
   * 3. Put the instances into the sub-blocks.
   * */
  void splitInstances();

  /**
   * This function is responsible for creating sub blocks (or dies) in a
   * multi-die design. For each technology in the design database, a new die is
   * created with a unique name. The first technology, which is the one parsed
   * at the TCL level, is skipped.
   */
  void makeSubBlocks();

  /**
   * Below functions are for detail placement.
   * */
  enum WHICH_DIE
  {
    TOP,
    BOTTOM
  };
  void switchInstancesToAssignedDie();
  void makeInterconnections(odb::dbBlock* lowerBlock, odb::dbBlock* upperBlock);
  void inheritRows(odb::dbBlock* parentBlock, odb::dbBlock* childBlock);
  void makeIOPinInterconnections();
  void rowConstruction(int dieWidth,
                       int dieHeight,
                       odb::dbBlock* topHeirBlock,
                       odb::dbBlock* childBlock1,
                       odb::dbBlock* childBlock2,
                       odb::dbLib* lib1,
                       odb::dbLib* lib2,
                       const odb::dbInst* inst1,
                       const odb::dbInst* inst2) const;

  odb::dbDatabase* db_{};
  utl::Logger* logger_{};
  par::PartitionMgr* partitionMgr_{};
  gpl::Replace* replace_{};
  dpl::Opendp* opendp_{};
  sta::dbSta* sta_{};

  TestCaseManager* testCaseManager_{};
  int numberOfDie_{};
  float shrinkAreaRatio_{};
  std::vector<float> shrinkLengthRatios_;
  std::string partitionFile_;
};

}  // namespace mdm

#endif  // OPENROAD_SRC_MDM_INCLUDE_MDM_MULTIDIEMANAGER_H_
