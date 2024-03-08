///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "SemiLegalizer.h"
namespace mdm {

void SemiLegalizer::run(bool useAbacus, char* targetDie)
{
  if (useAbacus) {
    runAbacus(targetDie);
  } else {
    doSimpleLegalize();
  }
}
void SemiLegalizer::runAbacus(char* targetDieChar, bool topHierDie)
{
  std::string targetDie{targetDieChar};
  if (targetDie == "top") {
    auto block = *db_->getChip()->getBlock()->getChildren().begin();
    runAbacus(block);
  } else if (targetDie == "bottom") {
    auto blockIter = db_->getChip()->getBlock()->getChildren().begin();
    std::advance(blockIter, 1);
    runAbacus(*blockIter);
  } else {
    if (topHierDie) {
      runAbacus(db_->getChip()->getBlock());
    }
    for (auto block : db_->getChip()->getBlock()->getChildren()) {
      runAbacus(block);
    }
  }
}

/*
  Sort cells according to x - position;
  1 foreach cell i do
  2   cbest ← ∞;
  3   for each row r do
  4     Insert cell i into row r;
  5     PlaceRow r(trial);
  6     Determine cost c;
  7     if c < cbest then cbest = c, rbest = r;
  8     Remove cell i from row r;
  9   end
  10  Insert Cell i to row rbest;
  11  PlaceRow rbest(final);
  12 end
*/
void SemiLegalizer::runAbacus(odb::dbBlock* block)
{
  targetBlock_ = block;

  std::multiset<odb::dbInst*,
                std::function<bool(const odb::dbInst*, const odb::dbInst*)>>
      instSet(targetBlock_->getInsts().begin(),
              targetBlock_->getInsts().end(),
              [](const odb::dbInst* a, const odb::dbInst* b) {
                return a->getLocation().x() < b->getLocation().x();
              });
  std::vector<instInRow> rowSet(targetBlock_->getRows().size());
  int yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  int rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();
  for (auto inst : instSet) {
    int CostBest = std::numeric_limits<int>::max();
    int rowBest = 0;  // usage: rowSet.at(rowBest);
    std::pair<int, int> originalLocation
        = {inst->getLocation().x(), inst->getLocation().y()};
    int nextSearchDownRow = (originalLocation.second - yMin) / rowHeight;
    int nextSearchUpRow = nextSearchDownRow + 1;
    while (nextSearchUpRow < rowSet.size() || nextSearchDownRow >= 0) {
      if (nextSearchDownRow < 0
          || (abs(yMin + rowHeight * nextSearchUpRow - originalLocation.first)
                  < abs(yMin + rowHeight * nextSearchDownRow
                        - originalLocation.first)
              && nextSearchUpRow < rowSet.size())) {  // search up
        if (abs(yMin + rowHeight * nextSearchUpRow - originalLocation.second)
            > CostBest) {
          nextSearchUpRow = rowSet.size();
          continue;
        }
        inst->setLocation(originalLocation.first,
                          yMin + rowHeight * nextSearchUpRow);
        rowSet.at(nextSearchUpRow).push_back(inst);
        if (degreeOfExcess(rowSet.at(nextSearchUpRow)) > 0) {
          rowSet.at(nextSearchUpRow).pop_back();
          nextSearchUpRow++;
          continue;
        }
        placeRow(rowSet.at(nextSearchUpRow));
        int cost
            = std::abs(inst->getLocation().x() - originalLocation.first)
              + std::abs(inst->getLocation().y() - originalLocation.second);
        if (cost < CostBest) {
          CostBest = cost;
          rowBest = nextSearchUpRow;
        }
        rowSet.at(nextSearchUpRow).pop_back();
        nextSearchUpRow++;
      } else {  // search down
        if (abs(yMin + rowHeight * nextSearchDownRow - originalLocation.second)
            > CostBest) {
          nextSearchDownRow = -1;
          continue;
        }
        inst->setLocation(originalLocation.first,
                          yMin + rowHeight * nextSearchDownRow);
        rowSet.at(nextSearchDownRow).push_back(inst);
        if (degreeOfExcess(rowSet.at(nextSearchDownRow)) > 0) {
          rowSet.at(nextSearchDownRow).pop_back();
          nextSearchDownRow--;
          continue;
        }
        placeRow(rowSet.at(nextSearchDownRow));
        int cost
            = std::abs(inst->getLocation().x() - originalLocation.first)
              + std::abs(inst->getLocation().y() - originalLocation.second);
        if (cost < CostBest) {
          CostBest = cost;
          rowBest = nextSearchDownRow;
        }
        rowSet.at(nextSearchDownRow).pop_back();
        nextSearchDownRow--;
      }
    }
    inst->setLocation(originalLocation.first, yMin + rowHeight * rowBest);
    rowSet.at(rowBest).push_back(inst);
    placeRow(rowSet.at(rowBest));
  }
}

void SemiLegalizer::placeRow(SemiLegalizer::instInRow row)
{
  // init cluster
  std::vector<AbacusCluster> abacusClusters;
  abacusClusters.reserve(row.size());
  auto numIntsInRow = row.size();
  // kernel algorithm for placeRow
  for (int i = 0; i < numIntsInRow; ++i) {
    auto inst = row.at(i);
    double instX = static_cast<double>(inst->getLocation().x());
    if (abacusClusters.empty()
        || abacusClusters.back().x + abacusClusters.back().w <= instX) {
      AbacusCluster cluster;  // create new cluster
      cluster.e = 0;
      cluster.w = 0;
      cluster.q = 0;
      cluster.x = instX;
      addCell(&cluster, inst);
      abacusClusters.push_back(cluster);
    } else {
      auto& lastCluster = abacusClusters.back();
      addCell(&lastCluster, inst);
      collapse(&lastCluster, abacusClusters);
    }
  }

  // transform cluster positions x_c(c) to cell positions x(i)
  for (const auto& cluster : abacusClusters) {
    int x = static_cast<int>(cluster.x);
    for (auto inst : cluster.instSet) {
      inst->setLocation(x, inst->getLocation().y());
      x += static_cast<int>(inst->getMaster()->getWidth());
    }
  }
}

void SemiLegalizer::addCell(SemiLegalizer::AbacusCluster* cluster,
                            odb::dbInst* inst)
{
  auto instX = static_cast<double>(inst->getLocation().x());
  auto instWidth = static_cast<double>(inst->getMaster()->getWidth());
  double instE = 1;  // we do not consider the fixed objects
  cluster->instSet.push_back(inst);
  cluster->e += instE;
  cluster->q += instE * (instX - cluster->w);
  cluster->w += instWidth;
}

void SemiLegalizer::addCluster(SemiLegalizer::AbacusCluster* predecessor,
                               SemiLegalizer::AbacusCluster* cluster)
{
  predecessor->instSet.insert(predecessor->instSet.end(),
                              cluster->instSet.begin(),
                              cluster->instSet.end());
  predecessor->e += cluster->e;
  predecessor->q += cluster->q - cluster->e * predecessor->w;
  predecessor->w += cluster->w;
}

void SemiLegalizer::collapse(SemiLegalizer::AbacusCluster* cluster,
                             std::vector<AbacusCluster>& abacusClusters)
{
  auto xMin = (*targetBlock_->getRows().begin())->getBBox().xMin();
  auto xMax = (*targetBlock_->getRows().begin())->getBBox().xMax();

  cluster->x = cluster->q / cluster->e;  // place cluster c
  // limit position between x min and x max - w_c(c)
  if (cluster->x < xMin) {
    cluster->x = xMin;
  } else if (cluster->x > xMax - cluster->w) {
    cluster->x = xMax - cluster->w;
  }

  if (abacusClusters.size() > 1) {
    // if predecessor exists,
    auto predecessor = &*(abacusClusters.end() - 2);
    if (predecessor->x + predecessor->w > cluster->x) {
      // merge cluster c to c'
      addCluster(predecessor, cluster);
      abacusClusters.erase(abacusClusters.end() - 1);  // remove cluster c
      collapse(predecessor, abacusClusters);
    }
  }
}

void SemiLegalizer::doSimpleLegalize(bool topHierDie)
{
  if (topHierDie) {
    doSimpleLegalize(db_->getChip()->getBlock());
  }
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    doSimpleLegalize(block);
  }
}
void SemiLegalizer::doSimpleLegalize(odb::dbBlock* block)
{
  targetBlock_ = block;
  if (!utilCheck()) {
    return;
  }
  adjustRowCapacity();
  shiftLegalize();
}
void SemiLegalizer::clingingRow()
{
  std::vector<RowCluster> rowClusters;
  auto numRows = targetBlock_->getRows().size();
  auto rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();

  rowClusters.resize(numRows);
  auto instSet = targetBlock_->getInsts();

  int yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  for (auto inst : instSet) {
    auto instY = inst->getLocation().y();
    int rowIdx = (instY - yMin) / rowHeight;
    inst->setLocation(inst->getLocation().x(), rowIdx * rowHeight + yMin);
    if (rowIdx >= 0 && rowIdx < numRows) {
      rowClusters[rowIdx].push_back(inst);
    } else {
      assert(0);
    }
  }

  for (auto& rowCluster : rowClusters) {
    if (rowCluster.empty()) {
      continue;
    }
    std::sort(rowCluster.begin(),
              rowCluster.end(),
              [](const odb::dbInst* a, const odb::dbInst* b) {
                return a->getLocation().x() < b->getLocation().x();
              });

    // set the start point

    auto dieWidth = db_->getChip()->getBlock()->getDieArea().dx();
    int powerDirection = 0;
    int powerFactors = 0;
    int widthSum = 0;

    for (auto inst : rowCluster) {
      widthSum += inst->getMaster()->getWidth();
      for (auto term : inst->getITerms()) {
        if (!term->getNet()) {
          continue;
        }
        auto net = term->getNet();
        powerDirection
            = net->getTermBBox().xCenter() - term->getBBox().xCenter();
        powerFactors += 1;
      }
    }
    powerDirection /= powerFactors;
    auto powerRatio = powerDirection / dieWidth;

    auto margin = dieWidth - widthSum;
    auto marginHalf = margin / 2;
    int startPoint = marginHalf - marginHalf * powerRatio;
    int cursor = startPoint;
    for (auto inst : rowCluster) {
      inst->setLocation(cursor, inst->getLocation().y());
      cursor += inst->getMaster()->getWidth();
    }
  }
}
void SemiLegalizer::shiftLegalize()
{
  /**
   * 1. Place cells from left to right considering overlap
   * 2. If the cell over the die, then shift the cell cluster to left.
   * */
  std::vector<RowCluster> rowClusters;
  auto numRows = targetBlock_->getRows().size();
  auto rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();

  rowClusters.resize(numRows);
  auto instSet = targetBlock_->getInsts();

  int yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  for (auto inst : instSet) {
    auto instY = inst->getLocation().y();
    int rowIdx = (instY - yMin) / rowHeight;
    inst->setLocation(inst->getLocation().x(), rowIdx * rowHeight + yMin);
    if (rowIdx >= 0 && rowIdx < numRows) {
      rowClusters[rowIdx].push_back(inst);
    } else {
      assert(0);
    }
  }

  for (auto& rowCluster : rowClusters) {
    std::sort(rowCluster.begin(),
              rowCluster.end(),
              [](const odb::dbInst* a, const odb::dbInst* b) {
                return a->getLocation().x() < b->getLocation().x();
              });
    int cursor = targetBlock_->getDieArea().xMin();
    int cellRowIdx = 0;
    for (auto inst : rowCluster) {
      if (cursor > inst->getLocation().x()) {
        // the target cell is overlapped -> shift until not overlapped
        inst->setLocation(cursor, inst->getLocation().y());
      }
      cursor = inst->getBBox()->xMax();

      /* If the cell over the row, then shift the cell cluster to left. */
      if (cursor > targetBlock_->getDieArea().xMax()) {
        // current instance is out from the die
        shiftCellsToLeft(rowCluster, inst, cellRowIdx);
      }
      cellRowIdx++;
    }
  }
}
void SemiLegalizer::shiftCellsToLeft(SemiLegalizer::RowCluster& cellRow,
                                     odb::dbInst* inst,
                                     int idx)
{
  /* Get the shift candidates
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
  std::vector<odb::dbInst*> candidates;
  int stickOutWidth
      = inst->getBBox()->xMax() - targetBlock_->getDieArea().xMax();
  int sumOfSpace = 0;
  int spacing;
  int cursor = inst->getLocation().x();
  candidates.push_back(inst);
  while (sumOfSpace < stickOutWidth) {
    idx -= 1;
    if (idx == -1) {
      spacing = cursor - 0;
      candidates.push_back(nullptr);
    } else {
      inst = cellRow.at(idx);
      spacing = cursor - inst->getBBox()->xMax();
      candidates.push_back(inst);
      cursor = inst->getLocation().x();
    }
    sumOfSpace += spacing;
  }
  candidates.pop_back();

  // Place candidates cells from right to left
  cursor = targetBlock_->getDieArea().xMax();
  for (auto inst : candidates) {
    int cellWidth = inst->getMaster()->getWidth();
    int placePoint = cursor - cellWidth;
    inst->setLocation(placePoint, inst->getLocation().y());
    cursor = inst->getLocation().x();
  }
}
void SemiLegalizer::adjustRowCapacity()
{
  // [.] Setting the Row Clusters
  std::vector<RowCluster> rowClusters;
  auto numRows = targetBlock_->getRows().size();
  auto rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();
  auto rowWidth = (*targetBlock_->getRows().begin())->getBBox().dx();

  rowClusters.resize(numRows);
  auto instSet = targetBlock_->getInsts();

  int yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  for (auto inst : instSet) {
    auto instY = inst->getLocation().y();
    int rowIdx = (instY - yMin) / rowHeight;
    inst->setLocation(inst->getLocation().x(), rowIdx * rowHeight + yMin);
    if (rowIdx >= 0 && rowIdx < numRows) {
      rowClusters[rowIdx].push_back(inst);
    } else {
      assert(0);
    }
  }
  // sort with x coordinate of instances for each row
  for (auto& rowCluster : rowClusters) {
    std::sort(rowCluster.begin(),
              rowCluster.end(),
              [](const odb::dbInst* a, const odb::dbInst* b) {
                return a->getLocation().x() < b->getLocation().x();
              });
  }
  // [.] Check capacity for the rows and adjust them
  // Scan the rows from bottom to top.
  // If it is not solved all, then
  // scan the rows from top the bottom.
  // Repeat above.
  enum DIRECTION
  {
    UPWARD,
    DOWNWARD
  };
  bool direction = UPWARD;
  bool solved = false;
  while (!solved) {
    solved = true;

    int rowClusterMaxIdx = static_cast<int>(rowClusters.size());
    int startIdx = direction == UPWARD ? 0 : rowClusterMaxIdx - 1;
    int endIdx = direction == UPWARD ? rowClusterMaxIdx - 1 : 0;
    int step = direction == UPWARD ? 1 : -1;

    for (int i = startIdx; direction == UPWARD ? (i < endIdx) : (i > endIdx);
         i += step) {
      auto& rowCluster = rowClusters.at(i);
      int excess = degreeOfExcess(rowCluster);
      if (excess > 0) {
        solved = false;
      }
      uint reducedWidth = 0;
      while (excess > reducedWidth) {
        auto smallestInst = rowCluster.at(0);
        rowCluster.erase(rowCluster.begin());
        int targetRowIdx;
        targetRowIdx = direction == UPWARD ? i + 1 : i - 1;
        rowClusters.at(targetRowIdx).push_back(smallestInst);
        smallestInst->setLocation(smallestInst->getLocation().x(),
                                  targetRowIdx * rowHeight + yMin);
        reducedWidth += smallestInst->getMaster()->getWidth();
      }
    }
    direction = direction == UPWARD ? DOWNWARD : UPWARD;
  }
}
int SemiLegalizer::degreeOfExcess(const std::vector<odb::dbInst*>& row)
{
  int rowWidth = (*targetBlock_->getRows().begin())->getBBox().dx();
  int totalWidth = std::accumulate(
      row.begin(), row.end(), 0, [](int sum, odb::dbInst* inst) {
        return sum + inst->getMaster()->getWidth();
      });
  return totalWidth > rowWidth ? totalWidth - rowWidth : 0;
}
bool SemiLegalizer::utilCheck()
{
  int64_t sumArea = std::accumulate(
      targetBlock_->getInsts().begin(),
      targetBlock_->getInsts().end(),
      0,
      [](int64_t sum, odb::dbInst* inst) {
        return sum
               + inst->getMaster()->getWidth() * inst->getMaster()->getHeight();
      });
  return sumArea <= targetBlock_->getDieArea().area();
}
}  // namespace mdm