#ifndef CACHEINTERFACE_H
#define CACHEINTERFACE_H

#include <deque>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "SSEConfig.h"

using namespace std;

class SSEEvent;

class CacheInterface {
  public:
    virtual void CacheEvent(SSEEvent& event)=0;
    virtual deque<string> GetEventsSinceId(string lastId)=0;
    virtual deque<string> GetAllEvents()=0;
    virtual size_t GetSizeOfCachedEvents()=0;
    ChannelConfig _config;
};
#endif
