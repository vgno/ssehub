#ifndef MEMORY_H
#define MEMORY_H

#include "CacheInterface.h"

class Memory : public CacheInterface {
  public:
    Memory(const ChannelConfig& config);
    void CacheEvent(SSEEvent& event);
    deque<string> GetEventsSinceId(string lastId);
    deque<string> GetAllEvents();
    size_t GetSizeOfCachedEvents();
    const ChannelConfig& _config;

  private:
    deque<string> _cache_keys;
    map<string, string> _cache_data;
};
#endif
