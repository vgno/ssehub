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
typedef boost::shared_ptr<SSEClient> SSEClientPtr;
typedef list<SSEClientPtr> SSEClientPtrList;

struct SSEChannelStats {
  long num_clients;
  int  num_cached_events;
  long num_broadcasted_events;
  long num_errors;
  long num_connects;
  long num_disconnects;
  int  cache_size;
};

class SSEChannel {
  public:
    SSEChannel(ChannelConfig& conf, string id);
    ~SSEChannel();
    string GetId();
    void Broadcast(const string& data);
    void BroadcastEvent(SSEEvent* event);
    void CacheEvent(SSEEvent* event);
    void SendEventsSince(SSEClient* client, string lastId);
    const SSEChannelStats& GetStats();
    void AddClient(SSEClient* client, HTTPRequest* req);
 
  private:
    string _id;
    int _efd;
    SSEClientPtrList _clientlist;
    ClientHandlerList::iterator curthread;
    ChannelConfig _config;
    SSEChannelStats _stats;
    boost::thread _cleanupthread;
    boost::thread _pingthread;
    deque<string> _cache_keys;
    map<string, SSEEventPtr> _cache_data;
    ClientHandlerList _clientpool;
    bool _allow_all_origins;
    char _evs_preamble_data[2052];

    void InitializeThreads();

    void CleanupMain();
    void CleanupThreads();
    void RemoveClient(SSEClient* client);
    void Ping();
    void SetCorsHeaders(HTTPRequest* req, HTTPResponse& res);
};

typedef boost::shared_ptr<SSEChannel> SSEChannelPtr;

#endif
