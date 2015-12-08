#include "Common.h"
#include "CacheAdapters/Memory.h"
#include "SSEConfig.h"
#include "SSEEvent.h"
#include <boost/foreach.hpp>

using namespace std;

Memory::Memory(const ChannelConfig& config) : _config(config) {}

void Memory::CacheEvent(SSEEvent& event) {
  // If we have the event id in our vector already don't remove it.
  // We want to keep the order even if we get an update on the event.
  if (std::find(_cache_keys.begin(), _cache_keys.end(), event.getid()) == _cache_keys.end()) {
    _cache_keys.push_back(event.getid());
  }

  _cache_data[event.getid()] = event.get();

  // Delete the oldest cache object if we hit the historyLength limit.
  if (_cache_keys.size() > _config.cacheLength) {
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
    events.push_back(_cache_data[*it]);
    it++;
  }

  return events;
}

deque<string> Memory::GetAllEvents() {
  deque<string> events;

	BOOST_FOREACH(const string& key, _cache_keys) {
		events.push_back(_cache_data[key]);
	}

  return events;
}

size_t Memory::GetSizeOfCachedEvents() {
    return _cache_keys.size();
}
