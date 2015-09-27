#include "Common.h"
#include "CacheAdapters/LevelDB.h"
#include "SSEConfig.h"
#include "SSEEvent.h"

using namespace std;

LevelDB::LevelDB(ChannelConfig config) {
  _config = config;
  InitDB("level.db");
  _cachesize = GetSizeOfCachedEvents();
}

LevelDB::~LevelDB() {
  leveldb_writeoptions_destroy(_woptions);
  leveldb_close(_db);
}

void LevelDB::InitDB(const string& dbfile) {
  char* err = NULL;

  _options = leveldb_options_create();
  _woptions = leveldb_writeoptions_create();
  _db = leveldb_open(_options, dbfile.c_str(), &err);

  LOG_IF(FATAL, err != NULL) << "Failed to open leveldb storage file.";

  leveldb_free(err);
  err = NULL;
}

void LevelDB::CacheEvent(SSEEvent* event) {
  char* err;

  leveldb_put(_db, _woptions, event->getid().c_str(), event->getid().length(), 
      event->get().c_str(), event->get().length(), &err);

  if (err != NULL) {
    LOG(ERROR) << "Failed to cache event with id " << event->getid();
    leveldb_free(err);
    return;
  }

  _cachesize++;
}

deque<string> LevelDB::GetEventsSinceId(string lastId) {
  deque<string> events;
  leveldb_iterator_t* it;
  leveldb_readoptions_t* readopts;
  const leveldb_snapshot_t* snapshot;

  snapshot = leveldb_create_snapshot(_db);
  readopts = leveldb_readoptions_create();
  leveldb_readoptions_set_snapshot(readopts, snapshot); 

  it = leveldb_create_iterator(_db, readopts);

  for (leveldb_iter_seek(it, lastId.c_str(), lastId.length()); 
      leveldb_iter_valid(it); leveldb_iter_next(it)) {
    size_t vlen;
    const char* val = leveldb_iter_value(it, &vlen);
    events.push_back(val);
  }

  leveldb_iter_destroy(it);
  leveldb_release_snapshot(_db, snapshot);
  leveldb_readoptions_destroy(readopts);
  return events;
}

deque<string> LevelDB::GetAllEvents() {
  deque<string> events;
  leveldb_iterator_t* it;
  leveldb_readoptions_t* readopts;
  const leveldb_snapshot_t* snapshot;

  snapshot = leveldb_create_snapshot(_db);
  readopts = leveldb_readoptions_create();
  leveldb_readoptions_set_snapshot(readopts, snapshot); 

  it = leveldb_create_iterator(_db, readopts);


  for (leveldb_iter_seek_to_first(it); leveldb_iter_valid(it); leveldb_iter_next(it)) {
    size_t vlen;
    const char* val = leveldb_iter_value(it, &vlen);
    events.push_back(val);
  }

  leveldb_iter_destroy(it);
  leveldb_release_snapshot(_db, snapshot);
  leveldb_readoptions_destroy(readopts);

  return events;
}

int LevelDB::GetSizeOfCachedEvents() {
  leveldb_iterator_t* it;
  leveldb_readoptions_t* readopts;
  const leveldb_snapshot_t* snapshot;
  int n_keys;

  snapshot = leveldb_create_snapshot(_db);

  readopts = leveldb_readoptions_create();
  it = leveldb_create_iterator(_db, readopts);
  leveldb_iter_seek_to_first(it);
  for(n_keys = 0; leveldb_iter_valid(it); n_keys++);

  leveldb_iter_destroy(it);
  leveldb_release_snapshot(_db, snapshot);
  leveldb_readoptions_destroy(readopts);

  return n_keys;
}
