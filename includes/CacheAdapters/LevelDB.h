#ifndef LevelDB_H
#define LevelDB_H

#include "leveldb/c.h"
#include "CacheInterface.h"

class LevelDB : public CacheInterface {
  public:
    LevelDB(const ChannelConfig& config);
    ~LevelDB();
    void InitDB(const string& dbfile); 
    void CacheEvent(SSEEvent& event);
    deque<string> GetEventsSinceId(string lastId);
    deque<string> GetAllEvents();
    size_t GetSizeOfCachedEvents();
    const ChannelConfig& _config;

  private:
    leveldb_t* _db;
    leveldb_options_t* _options; 
    leveldb_writeoptions_t* _woptions;
    leveldb_readoptions_t* _roptions;
};
#endif
