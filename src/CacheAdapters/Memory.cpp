#include "Common.h"
#include "CacheAdapters/Memory.h"
#include "SSEConfig.h"
#include "SSEEvent.h"

using namespace std;

Memory::Memory(ChannelConfig config) {
    _config = config;
}

void Memory::CacheEvent(SSEEvent* event) {
  // If we have the event id in our vector already don't remove it.
  // We want to keep the order even if we get an update on the event.
  if (std::find(_cache_keys.begin(), _cache_keys.end(), event->getid()) == _cache_keys.end()) {
    _cache_keys.push_back(event->getid());
  }

  _cache_data[event->getid()] = SSEEventPtr(event);

  // Delete the oldest cache object if we hit the historyLength limit.
  if ((int)_cache_keys.size() > _config.cacheLength) {
    string &firstElementId = *(_cache_keys.begin());
    _cache_data.erase(firstElementId);
    _cache_keys.erase(_cache_keys.begin());
  }
}

deque<string> Memory::GetEventsSinceId(string lastId) {
  deque<string>::const_iterator it;
  deque<string> events;

  it = std::find(_cache_keys.begin(), _cache_keys.end(), lastId);

  while (it != _cache_keys.end()) {
    events.push_back(_cache_data[*(it)]->get());
    it++;
  }

  return events;
}

deque<string> Memory::GetAllEvents() {
  deque<string>::const_iterator it;
  deque<string> events;

  it = _cache_keys.begin();
  if (it == _cache_keys.end()) return events;

  do {
    events.push_back(_cache_data[*(it)]->get());
  } while((it++) != _cache_keys.end());

  return events;
}

int Memory::GetSizeOfCachedEvents() {
    return (int) _cache_keys.size();
}
