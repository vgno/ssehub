#ifndef REDIS_H
#define REDIS_H

#include "CacheInterface.h"
#include <redisclient/redissyncclient.h>

using namespace std;

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
    RedisSyncClient* GetClient();
    string Lookup(string hostname);
    deque<RedisSyncClient *> _clients;
    string _key;
    deque<RedisSyncClient *>::iterator _curclient;
};
#endif
