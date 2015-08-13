#ifndef REDIS_H
#define REDIS_H

#include "CacheInterface.h"
#include <redisclient/redissyncclient.h>

class Redis : public CacheInterface {
  public:
    Redis(string key, ChannelConfig config);
    void CacheEvent(SSEEvent* event);
    deque<string> GetEventsSinceId(string lastId);
    deque<string> GetAllEvents();
    int GetSizeOfCachedEvents();
    ChannelConfig _config;

  private:
    void Connect();
    void Disconnect();
    void Reconnect(int delay);
    void Expire(int ttl);
    RedisSyncClient* _client;
    string _key;
};
#endif
