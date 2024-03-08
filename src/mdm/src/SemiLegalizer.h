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

#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace mdm {
class MultiDieManager;
class SemiLegalizer
{
  struct AbacusCluster
  {
    std::vector<odb::dbInst*> instSet;
    SemiLegalizer::AbacusCluster* predecessor;
    SemiLegalizer::AbacusCluster* successor;

    double e;  // weight of displacement in the objective
    double q;  // x = q/e
    double w;  // cluster width
    double x;  // optimal location (cluster's left edge coordinate)
  };
  using RowCluster = std::vector<odb::dbInst*>;
  using instInRow = std::vector<odb::dbInst*>;

 public:
  void setDb(odb::dbDatabase* db) { db_ = db; }
  void run(bool useAbacus = true, char* targetDie = "");

 private:
  /**
   * \Refer:
   * [Abacus] Fast Legalization of Standard Cell Circuits with Minimal Movement
   * [DREAMPlace]
   * https://github.com/limbo018/DREAMPlace/tree/master/dreamplace/ops/abacus_legalize
   * This is the simple and fast legalizer
   * */
  void runAbacus(char* targetDieChar = "", bool topHierDie = false);
  /**
   *  Here, we do not implement this considering fixed objects
   *  All object will be considered as movable.
   *  Considering fixed object will be remained as to-do thing.
   * */
  void runAbacus(odb::dbBlock* block);

  void placeRow(instInRow row);
  void addCell(AbacusCluster* cluster, odb::dbInst* inst);
  void collapse(SemiLegalizer::AbacusCluster* cluster,
                std::vector<AbacusCluster>& predecessor);

  /**
   * Assume it has single height rows
   * This will adjust the row capacity
   * */
  void doSimpleLegalize(bool topHierDie = false);
  void doSimpleLegalize(odb::dbBlock* block);

  /**
   * 1. Place cells from left to right considering overlap
   * 2. If the cell over the die, then shift the cell cluster to left.
   * */
  void shiftLegalize();
  /**
   * Get the shift candidates
   * `candidates` means,
   * ---------------------------------------------------------
   * ... ┌─────┐      ┌─────┐------┌─────┐-----┌─────┐---┌──────────────┐
   * ... │     │      │ c3  │      │ c2  │     │ c1  │   │              │
   * ... └─────┘      └─────┘------└─────┘-----└─────┘---└──────────────┘
   *                         <---->       <--->       <->    <---------->
   *                        spacing3     spacing2   spacing1 stickOutWidth
   *                                <------------------->
   *                                 c1, c2 are candidates
   *                         because Σ spacing_i < stickOutWidth until c3
   * ---------------------------------------------------------
   *                                                         |
   *                                                    Die end point
   * */
  void shiftCellsToLeft(RowCluster& cellRow, odb::dbInst* inst, int idx);
  int degreeOfExcess(const std::vector<odb::dbInst*>& row);

  bool utilCheck();
  void adjustRowCapacity();

  /**
   * \deprecated
   * This function attaches the instances in the same row tightly together,
   * considering the wire length force
   * */
  void clingingRow();

  odb::dbDatabase* db_;
  odb::dbBlock* targetBlock_;
  void addCluster(SemiLegalizer::AbacusCluster* predecessor,
                  SemiLegalizer::AbacusCluster* cluster);
};
}  // namespace mdm
