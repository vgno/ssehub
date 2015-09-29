#include "Common.h"
#include "CacheAdapters/LevelDB.h"
#include "SSEConfig.h"
#include "SSEEvent.h"

using namespace std;

LevelDB::LevelDB(const ChannelConfig& config) : _config(config) {
  const string cachefile = _config.server->GetValue("leveldb.storageDir") + "/" + config.id + ".db";
  LOG(INFO) << "LevelDB storage file: " << cachefile;
  InitDB(cachefile);
}

LevelDB::~LevelDB() {
  leveldb_options_destroy(_options);
  leveldb_writeoptions_destroy(_woptions);
  leveldb_readoptions_destroy(_roptions);
  leveldb_close(_db);
}

void LevelDB::InitDB(const string& dbfile) {
  char* err = NULL;

  _options = leveldb_options_create();
  _roptions = leveldb_readoptions_create();
  _woptions = leveldb_writeoptions_create();
  leveldb_options_set_create_if_missing(_options, 1);
  _db = leveldb_open(_options, dbfile.c_str(), &err);

  if (err != NULL) {
    LOG(FATAL) << "Failed to open leveldb storage file: " << err;
    leveldb_free(err);
    err = NULL;
  }

  LOG(INFO) << "LevelDB::InitDB finished for " << _config.id;
}

void LevelDB::CacheEvent(SSEEvent* event) {
  char* err = NULL;

  leveldb_put(_db, _woptions, event->getid().c_str(), event->getid().length(), 
      event->get().c_str(), event->get().length(), &err);

  if (err != NULL) {
    LOG(ERROR) << "Failed to cache event with id " << event->getid() << ": " << err;
    leveldb_free(err);
    return;
  }

  if (GetSizeOfCachedEvents() > _config.cacheLength) {
    size_t klen;
    char *err = NULL;
    leveldb_iterator_t* it = leveldb_create_iterator(_db, _roptions);

    leveldb_iter_seek_to_first(it);
    const string key = leveldb_iter_key(it, &klen);

    leveldb_delete(_db, _woptions, key.c_str(), klen, &err);

    if (err != NULL) {
      LOG(ERROR) << "Failed to delete key " << key << ": " << err;
      leveldb_free(err);
    } else {
      DLOG(INFO) << "Deleted key " << key;
    }

    leveldb_iter_destroy(it);
  }
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
  for(n_keys = 0; leveldb_iter_valid(it); leveldb_iter_next(it)) {
    n_keys++;
  }

  leveldb_iter_destroy(it);
  leveldb_release_snapshot(_db, snapshot);
  leveldb_readoptions_destroy(readopts);

  return n_keys;
}
