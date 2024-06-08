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
#include "mdm/MultiDieManager.hh"
#include "SemiLegalizer.h"
#include "dpl/Opendp.h"
#include "dpo/Optdp.h"
#include "odb/db.h"
// clang-format on

namespace mdm {
using namespace std;
void MultiDieManager::multiDieDPL()
{
  auto odp = new dpl::Opendp();
  odp->init(db_, logger_);
  odp->detailedPlacement(0, 0, "", false, db_->getChip()->getBlock());
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    odp->detailedPlacement(0, 0, "", false, block);
  }
}

void MultiDieManager::multiDieDPO()
{
  auto dpo = new dpo::Optdp;
  auto odp = new dpl::Opendp();
  odp->init(db_, logger_);
  dpo->init(db_, logger_, odp);
  dpo->improvePlacement(1, 0, 0, db_->getChip()->getBlock(), false);
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    dpo->improvePlacement(1, 0, 0, block, false);
  }
}

void MultiDieManager::runSemiLegalizer(char* targetDie)
{
  SemiLegalizer semiLegalizer{};
  semiLegalizer.setDb(db_);
  semiLegalizer.run(true, targetDie);
}

}  // namespace mdm