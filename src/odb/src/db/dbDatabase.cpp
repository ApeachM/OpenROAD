// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbDatabase.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "dbArrayTable.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbChip.h"
#include "dbGDSLib.h"
#include "dbITerm.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbNameCache.h"
#include "dbNet.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbRSeg.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbExtControl.h"
#include "odb/dbStream.h"
#include "utl/Logger.h"

namespace odb {

//
// Magic number is: ATHENADB
//
constexpr int DB_MAGIC1 = 0x41544845;  // ATHE
constexpr int DB_MAGIC2 = 0x4E414442;  // NADB

template class dbTable<_dbDatabase>;

static dbTable<_dbDatabase>* db_tbl = nullptr;
// Must be held to access db_tbl
static std::mutex* db_tbl_mutex = new std::mutex;
static std::atomic<uint> db_unique_id = 0;

bool _dbDatabase::operator==(const _dbDatabase& rhs) const
{
  //
  // For the time being the fields,
  // magic1, magic2, schema_major, schema_minor,
  // unique_id, and file,
  // are not used for comparison.
  //
  if (_master_id != rhs._master_id) {
    return false;
  }

  if (_chip != rhs._chip) {
    return false;
  }

  if (*_tech_tbl != *rhs._tech_tbl) {
    return false;
  }

  if (*_lib_tbl != *rhs._lib_tbl) {
    return false;
  }

  if (*_chip_tbl != *rhs._chip_tbl) {
    return false;
  }

  if (*_gds_lib_tbl != *rhs._gds_lib_tbl) {
    return false;
  }

  if (*_prop_tbl != *rhs._prop_tbl) {
    return false;
  }

  if (*_name_cache != *rhs._name_cache) {
    return false;
  }

  return true;
}

dbObjectTable* _dbDatabase::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbTechObj:
      return _tech_tbl;

    case dbLibObj:
      return _lib_tbl;

    case dbChipObj:
      return _chip_tbl;

    case dbGdsLibObj:
      return _gds_lib_tbl;

    case dbPropertyObj:
      return _prop_tbl;
    default:
      getLogger()->critical(
          utl::ODB,
          438,
          "Internal inconsistency: no table found for type {}",
          type);
      break;
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////////
//
// _dbDatabase - Methods
//
////////////////////////////////////////////////////////////////////

_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */)
{
  _magic1 = DB_MAGIC1;
  _magic2 = DB_MAGIC2;
  _schema_major = db_schema_major;
  _schema_minor = db_schema_minor;
  _master_id = 0;
  _logger = nullptr;
  _unique_id = db_unique_id++;

  _chip_tbl = new dbTable<_dbChip>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj, 2, 1);

  _gds_lib_tbl
      = new dbTable<_dbGDSLib>(this,
                               this,
                               (GetObjTbl_t) &_dbDatabase::getObjectTable,
                               dbGdsLibObj,
                               2,
                               1);

  _tech_tbl = new dbTable<_dbTech>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj, 2, 1);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  _prop_itr = new dbPropertyItr(_prop_tbl);
}

//
// This constructor is use by dbDatabase::clear(), so the the unique-id is
// reset.
//
_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */, int id)
{
  _magic1 = DB_MAGIC1;
  _magic2 = DB_MAGIC2;
  _schema_major = db_schema_major;
  _schema_minor = db_schema_minor;
  _master_id = 0;
  _logger = nullptr;
  _unique_id = id;

  _chip_tbl = new dbTable<_dbChip>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj, 2, 1);

  _gds_lib_tbl
      = new dbTable<_dbGDSLib>(this,
                               this,
                               (GetObjTbl_t) &_dbDatabase::getObjectTable,
                               dbGdsLibObj,
                               2,
                               1);

  _tech_tbl = new dbTable<_dbTech>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj, 2, 1);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  _prop_itr = new dbPropertyItr(_prop_tbl);
}

_dbDatabase::~_dbDatabase()
{
  delete _tech_tbl;
  delete _lib_tbl;
  delete _chip_tbl;
  delete _gds_lib_tbl;
  delete _prop_tbl;
  delete _name_cache;
  delete _prop_itr;
}

dbOStream& operator<<(dbOStream& stream, const _dbDatabase& db)
{
  dbOStreamScope scope(stream, "dbDatabase");
  stream << db._magic1;
  stream << db._magic2;
  stream << db._schema_major;
  stream << db._schema_minor;
  stream << db._master_id;
  stream << db._chip;
  stream << *db._tech_tbl;
  stream << *db._lib_tbl;
  stream << *db._chip_tbl;
  stream << *db._gds_lib_tbl;
  stream << NamedTable("prop_tbl", db._prop_tbl);
  stream << *db._name_cache;
  stream << *db._gds_lib_tbl;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbDatabase& db)
{
  stream >> db._magic1;

  if (db._magic1 != DB_MAGIC1) {
    throw std::runtime_error("database file is not an OpenDB Database");
  }

  stream >> db._magic2;

  if (db._magic2 != DB_MAGIC2) {
    throw std::runtime_error("database file is not an OpenDB Database");
  }

  stream >> db._schema_major;

  if (db._schema_major != db_schema_major) {
    throw std::runtime_error("Incompatible database schema revision");
  }

  stream >> db._schema_minor;

  if (db._schema_minor < db_schema_initial) {
    throw std::runtime_error("incompatible database schema revision");
  }

  if (db._schema_minor > db_schema_minor) {
    throw std::runtime_error(
        fmt::format("incompatible database schema revision {}.{} > {}.{}",
                    db._schema_major,
                    db._schema_minor,
                    db_schema_major,
                    db_schema_minor));
  }

  stream >> db._master_id;

  stream >> db._chip;

  dbId<_dbTech> old_db_tech;
  if (!db.isSchema(db_schema_block_tech)) {
    stream >> old_db_tech;
  }
  stream >> *db._tech_tbl;
  stream >> *db._lib_tbl;
  stream >> *db._chip_tbl;
  if (db.isSchema(db_schema_gds_lib_in_block)) {
    stream >> *db._gds_lib_tbl;
  }
  stream >> *db._prop_tbl;
  stream >> *db._name_cache;

  // Set the _tech on the block & libs now they are loaded
  if (!db.isSchema(db_schema_block_tech)) {
    if (db._chip) {
      _dbChip* chip = db._chip_tbl->getPtr(db._chip);
      if (chip->_top) {
        chip->_block_tbl->getPtr(chip->_top)->_tech = old_db_tech;
      }
    }

    auto db_public = (dbDatabase*) &db;
    for (auto lib : db_public->getLibs()) {
      _dbLib* lib_impl = (_dbLib*) lib;
      lib_impl->_tech = old_db_tech;
    }
  }

  // Fix up the owner id of properties of this db, this value changes.
  const uint oid = db.getId();

  for (_dbProperty* p : dbSet<_dbProperty>(&db, db._prop_tbl)) {
    p->_owner = oid;
  }

  // Set the revision of the database to the current revision
  db._schema_major = db_schema_major;
  db._schema_minor = db_schema_minor;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbDatabase - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbLib> dbDatabase::getLibs()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbLib>(db, db->_lib_tbl);
}

dbLib* dbDatabase::findLib(const char* name)
{
  for (dbLib* lib : getLibs()) {
    if (strcmp(lib->getConstName(), name) == 0) {
      return lib;
    }
  }

  return nullptr;
}

dbSet<dbTech> dbDatabase::getTechs()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbTech>(db, db->_tech_tbl);
}

dbTech* dbDatabase::findTech(const char* name)
{
  for (auto tech : getTechs()) {
    auto tech_impl = (_dbTech*) tech;
    if (tech_impl->_name == name) {
      return tech;
    }
  }

  return nullptr;
}

dbMaster* dbDatabase::findMaster(const char* name)
{
  for (dbLib* lib : getLibs()) {
    dbMaster* master = lib->findMaster(name);
    if (master) {
      return master;
    }
  }
  return nullptr;
}

// Remove unused masters
int dbDatabase::removeUnusedMasters()
{
  std::vector<dbMaster*> unused_masters;
  dbSet<dbLib> libs = getLibs();

  for (auto lib : libs) {
    dbSet<dbMaster> masters = lib->getMasters();
    // Collect all dbMasters for later comparision
    for (auto master : masters) {
      unused_masters.push_back(master);
    }
  }
  // Get instances from this Database
  dbChip* chip = getChip();
  dbBlock* block = chip->getBlock();
  dbSet<dbInst> insts = block->getInsts();

  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    // Filter out the master that matches inst_master
    auto masterIt
        = std::find(unused_masters.begin(), unused_masters.end(), master);
    if (masterIt != unused_masters.end()) {
      // erase used maseters from container
      unused_masters.erase(masterIt);
    }
  }
  // Destroy remaining unused masters
  for (auto& elem : unused_masters) {
    dbMaster::destroy(elem);
  }
  return unused_masters.size();
}

dbSet<dbChip> dbDatabase::getChips()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbChip>(db, db->_chip_tbl);
}

uint dbDatabase::getNumberOfMasters()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return db->_master_id;
}

dbChip* dbDatabase::getChip()
{
  _dbDatabase* db = (_dbDatabase*) this;

  if (db->_chip == 0) {
    return nullptr;
  }

  return (dbChip*) db->_chip_tbl->getPtr(db->_chip);
}

dbTech* dbDatabase::getTech()
{
  auto techs = getTechs();

  const int num_tech = techs.size();
  if (num_tech == 0) {
    return nullptr;
  }

  if (num_tech == 1) {
    return *techs.begin();
  }

  auto impl = (_dbDatabase*) this;
  impl->_logger->error(
      utl::ODB, 432, "getTech() is obsolete in a multi-tech db");
}

void dbDatabase::read(std::istream& file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbIStream stream(db, file);
  stream >> *db;
  ((dbDatabase*) db)->triggerPostReadDb();
}

void dbDatabase::write(std::ostream& file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *db;
  file.flush();
}

void dbDatabase::beginEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  {
    delete block->_journal;
  }

  block->_journal = new dbJournal(block_);
  assert(block->_journal);
}

void dbDatabase::endEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  dbJournal* eco = block->_journal;
  block->_journal = nullptr;

  {
    delete block->_journal_pending;
  }

  block->_journal_pending = eco;
}

bool dbDatabase::ecoEmpty(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal) {
    return block->_journal->empty();
  }

  return false;
}

int dbDatabase::checkEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal) {
    return block->_journal->size();
  }
  return 0;
}

void dbDatabase::readEco(dbBlock* block_, const char* filename)
{
  _dbBlock* block = (_dbBlock*) block_;

  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);
  file.open(filename, std::ios::binary);

  dbIStream stream(block->getDatabase(), file);
  dbJournal* eco = new dbJournal(block_);
  assert(eco);
  stream >> *eco;

  {
    delete block->_journal_pending;
  }

  block->_journal_pending = eco;
}

void dbDatabase::writeEco(dbBlock* block_, const char* filename)
{
  _dbBlock* block = (_dbBlock*) block_;

  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    int errnum = errno;
    block->getImpl()->getLogger()->error(
        utl::ODB, 2, "Error opening file {}", strerror(errnum));
    return;
  }

  file.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);

  if (block->_journal_pending) {
    dbOStream stream(block->getDatabase(), file);
    stream << *block->_journal_pending;
  }
}

void dbDatabase::commitEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  // TODO: Need a check to ensure the commit is not applied to the block of
  // which this eco was generated from.
  if (block->_journal_pending) {
    block->_journal_pending->redo();
    delete block->_journal_pending;
    block->_journal_pending = nullptr;
  }
}

void dbDatabase::undoEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal_pending) {
    block->_journal_pending->undo();
    delete block->_journal_pending;
    block->_journal_pending = nullptr;
  }
}

void dbDatabase::setLogger(utl::Logger* logger)
{
  _dbDatabase* _db = (_dbDatabase*) this;
  _db->_logger = logger;
}

dbDatabase* dbDatabase::create()
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  if (db_tbl == nullptr) {
    db_tbl = new dbTable<_dbDatabase>(
        nullptr, nullptr, (GetObjTbl_t) nullptr, dbDatabaseObj);
  }

  _dbDatabase* db = db_tbl->create();
  return (dbDatabase*) db;
}

void dbDatabase::clear()
{
  _dbDatabase* db = (_dbDatabase*) this;
  int id = db->_unique_id;
  db->~_dbDatabase();
  new (db) _dbDatabase(db, id);
}

void dbDatabase::destroy(dbDatabase* db_)
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  _dbDatabase* db = (_dbDatabase*) db_;
  db_tbl->destroy(db);
}

dbDatabase* dbDatabase::getDatabase(uint dbid)
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  return (dbDatabase*) db_tbl->getPtr(dbid);
}

dbDatabase* dbObject::getDb() const
{
  return (dbDatabase*) getImpl()->getDatabase();
}

utl::Logger* _dbDatabase::getLogger() const
{
  if (!_logger) {
    std::cerr << "[CRITICAL ODB-0001] No logger is installed in odb."
              << std::endl;
    exit(1);
  }
  return _logger;
}

utl::Logger* _dbObject::getLogger() const
{
  return getDatabase()->getLogger();
}

void _dbDatabase::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  _tech_tbl->collectMemInfo(info.children_["tech"]);
  _lib_tbl->collectMemInfo(info.children_["lib"]);
  _chip_tbl->collectMemInfo(info.children_["chip"]);
  _gds_lib_tbl->collectMemInfo(info.children_["gds_lib"]);
  _prop_tbl->collectMemInfo(info.children_["prop"]);
  _name_cache->collectMemInfo(info.children_["name_cache"]);
}

void dbDatabase::report()
{
  _dbDatabase* db = (_dbDatabase*) this;
  MemInfo root;
  db->collectMemInfo(root);
  utl::Logger* logger = db->getLogger();
  std::function<int64_t(MemInfo&, const std::string&, int)> print =
      [&](MemInfo& info, const std::string& name, int depth) {
        double avg_size = 0;
        int64_t total_size = info.size;
        if (info.cnt > 0) {
          avg_size = info.size / static_cast<double>(info.cnt);
        }

        logger->report("{:40s} cnt={:10d} size={:12d} (avg elem={:12.1f})",
                       name.c_str(),
                       info.cnt,
                       info.size,
                       avg_size);
        for (auto [name, child] : info.children_) {
          total_size += print(child, std::string(depth, ' ') + name, depth + 1);
        }
        return total_size;
      };
  auto total_size = print(root, "dbDatabase", 1);
  logger->report("Total size = {}", total_size);
}

void dbDatabase::addObserver(dbDatabaseObserver* observer)
{
  observer->setUnregisterObserver(
      [this, observer] { removeObserver(observer); });
  _dbDatabase* db = (_dbDatabase*) this;
  db->observers_.insert(observer);
}

void dbDatabase::removeObserver(dbDatabaseObserver* observer)
{
  observer->setUnregisterObserver(nullptr);
  _dbDatabase* db = (_dbDatabase*) this;
  db->observers_.erase(observer);
}

void dbDatabase::triggerPostReadLef(dbTech* tech, dbLib* library)
{
  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    observer->postReadLef(tech, library);
  }
}

void dbDatabase::triggerPostReadDef(dbBlock* block, const bool floorplan)
{
  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    if (floorplan) {
      observer->postReadFloorplanDef(block);
    } else {
      observer->postReadDef(block);
    }
  }
}

void dbDatabase::triggerPostReadDb()
{
  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    observer->postReadDb(this);
  }
}

}  // namespace odb
