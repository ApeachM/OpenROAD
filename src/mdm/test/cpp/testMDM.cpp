#include <gtest/gtest.h>

#include "db.h"
#include "mdm/MultiDieManager.hh"
#include "utl/Logger.h"

namespace mdm {

using namespace odb;
class mdmTest : public ::testing::Test
{
 protected:
  odb::dbDatabase* db_{};
  utl::Logger* logger_{};
  mdm::MultiDieManager* mdm_{};
  void SetUp() override
  {
    logger_ = new utl::Logger();
    db_ = dbDatabase::create();
    db_->setLogger(logger_);
    dbTech* tech_ = dbTech::create(db_, "tech");
    dbTechLayer::create(tech_, "L1", dbTechLayerType::MASTERSLICE);
    dbLib* lib = dbLib::create(db_, "lib1", tech_, ',');
    dbChip* chip_ = dbChip::create(db_);
    dbBlock* block_ = dbBlock::create(chip_, "block");

    mdm_ = new mdm::MultiDieManager();
  }

  void TearDown() override
  {
    delete mdm_;
    dbDatabase::destroy(db_);
  }
};

TEST_F(mdmTest, InitMultiDieManager)
{
  mdm_->init(db_, logger_, nullptr, nullptr, nullptr, nullptr);

  EXPECT_TRUE(mdm_);
}
}  // namespace mdm
