#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
#include <deque>
#include <map>
#include <string>
#include <glog/logging.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <pthread.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "Common.h"
#include "SSEConfig.h"
#include "CacheAdapters/Memory.h"
#include "CacheAdapters/Redis.h"
#include "CacheAdapters/LevelDB.h"

using namespace std;

// Forward declarations.
class SSEEvent;
class SSEClient;
class SSEClientHandler;
class HTTPRequest;
class HTTPResponse;

typedef boost::shared_ptr<SSEEvent> SSEEventPtr;
typedef boost::shared_ptr<SSEClientHandler> ClientHandlerPtr;
typedef vector<ClientHandlerPtr> ClientHandlerList;

struct SSEChannelStats {
  ulong num_clients;
  uint  num_cached_events;
  ulong num_broadcasted_events;
  ulong num_errors;
  ulong num_connects;
  ulong num_disconnects;
  uint  cache_size;
};

class SSEChannel {
  public:
    SSEChannel(ChannelConfig conf, string id);
    ~SSEChannel();
    string GetId();
    void Broadcast(const string& data);
    void BroadcastEvent(SSEEvent* event);
    void CacheEvent(SSEEvent* event);
    void SendEventsSince(SSEClient* client, string lastId);
    void SendCache(SSEClient* client);
    const SSEChannelStats& GetStats();
    void AddClient(SSEClient* client, HTTPRequest* req);
    ulong GetNumClients();

  private:
    int _efd;
    ClientHandlerList::iterator curthread;
    ChannelConfig _config;
    SSEChannelStats _stats;
    boost::thread _cleanupthread;
    boost::thread _pingthread;
    ClientHandlerList _clientpool;
    CacheInterface* _cache_adapter;
    bool _allow_all_origins;
    char _evs_preamble_data[2052];

    void InitializeCache();

    void InitializeThreads();

    void CleanupMain();
    void CleanupThreads();
    void Ping();
    void SetCorsHeaders(HTTPRequest* req, HTTPResponse& res);
};

typedef boost::shared_ptr<SSEChannel> SSEChannelPtr;

#endif
