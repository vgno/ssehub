#ifndef REDIS_H
#define REDIS_H

#include "CacheInterface.h"
#include <redisclient/redissyncclient.h>
#include <boost/shared_ptr.hpp>

using namespace std;

typedef boost::shared_ptr<RedisSyncClient> RedisSyncClientPtr;

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
    deque<RedisSyncClientPtr> _clients;
    string _key;
    deque<RedisSyncClientPtr>::iterator _curclient;
};
#endif
