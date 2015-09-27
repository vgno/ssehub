#ifndef LevelDB_H
#define LevelDB_H

#include "leveldb/c.h"
#include "CacheInterface.h"

class LevelDB : public CacheInterface {
  public:
    LevelDB(ChannelConfig config);
    ~LevelDB();
    void InitDB(const string& dbfile); 
    void CacheEvent(SSEEvent* event);
    deque<string> GetEventsSinceId(string lastId);
    deque<string> GetAllEvents();
    int GetSizeOfCachedEvents();
    ChannelConfig _config;

  private:
    leveldb_t* _db;
    leveldb_options_t* _options; 
    leveldb_writeoptions_t* _woptions;
    unsigned int _cachesize;
};
#endif
