#ifndef CACHEINTERACE_H
#define CACHEINTERFACE_H

#include <deque>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "SSEConfig.h"

using namespace std;

class SSEEvent;

typedef boost::shared_ptr<SSEEvent> SSEEventPtr;

class CacheInterface {
  public:
    void CacheEvent(SSEEvent* event);
    deque<string> GetEventsSinceId(string lastId);
    deque<string> GetAllEvents();
    int GetSizeOfCachedEvents();
    CacheConfig _config;
    deque<string> _cache_keys;
    map<string, SSEEventPtr> _cache_data;
};
#endif
