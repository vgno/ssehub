#ifndef REDIS_H
#define REDIS_H

#include "CacheInterface.h"
#include <redisclient/redissyncclient.h>

class Redis : public CacheInterface {
  public:
    Redis(string key, int length, int expires);
    void CacheEvent(SSEEvent* event);
    deque<string> GetEventsSinceId(string lastId);
    deque<string> GetAllEvents();
    int GetSizeOfCachedEvents();
    CacheConfig _config;

  private:
    void Connect();
    void Disconnect();
    void Reconnect(int delay);
    RedisSyncClient* _client;
    string _key;
};
#endif
