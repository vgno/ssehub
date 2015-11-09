#ifndef REDIS_H
#define REDIS_H

#include "CacheInterface.h"
#include <redisclient/redissyncclient.h>

using namespace std;


class Redis : public CacheInterface {
  public:
    Redis(const string key, const ChannelConfig& config);
    void CacheEvent(SSEEvent* event);
    deque<string> GetEventsSinceId(string lastId);
    deque<string> GetAllEvents();
    int GetSizeOfCachedEvents();
    const ChannelConfig& _config;

  private:
    void Expire(int ttl);
    bool InitClient(RedisSyncClient& client);
    string Lookup(string hostname);
    string _key;
    string _host;
    unsigned short _port;
};
#endif
