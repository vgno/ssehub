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
#include "SSEConfig.h"

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
  long num_clients;
  int  num_cached_events;
  long num_broadcasted_events;
  long long num_errors;
  long long num_connects;
  long long num_disconnects;
  int  cache_size;
};

class SSEChannel {
  public:
    SSEChannel(ChannelConfig& conf, string id);
    ~SSEChannel();
    string GetId();
    void AddClient(SSEClient* client, HTTPRequest* req);
    void Broadcast(const string& data);
    void BroadcastEvent(SSEEvent* event);
    void CacheEvent(SSEEvent* event);
    void SendEventsSince(SSEClient* client, string lastId);
    void GetStats(SSEChannelStats* stats);

  private:
    string id;
    long num_broadcasted_events;
    ClientHandlerList::iterator curthread;
    ChannelConfig config;
    SSEChannelStats _stats;
    pthread_t _pingthread;
    deque<string> cache_keys;
    map<string, SSEEventPtr> cache_data;
    ClientHandlerList clientpool;
    bool allowAllOrigins;
    char evs_preamble_data[2052];

    void InitializeThreads();
    void CleanupThreads();
    void Ping();
    static void* PingThread(void*);
    void SetCorsHeaders(HTTPRequest* req, HTTPResponse& res);
};

typedef boost::shared_ptr<SSEChannel> SSEChannelPtr;

#endif
