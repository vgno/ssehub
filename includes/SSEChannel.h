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

using namespace std;

// Forward declarations.
class SSEEvent;
class SSEClient;
class SSEConfig;
class SSEClientHandler;
class HTTPRequest;


typedef boost::shared_ptr<SSEEvent> SSEEventPtr;
typedef boost::shared_ptr<SSEClientHandler> ClientHandlerPtr;
typedef vector<ClientHandlerPtr> ClientHandlerList;

struct SSEChannelStats {
  long num_clients;
  int  num_cached_events;
  long num_broadcasted_events;
  int  cache_size;
};

class SSEChannel {
  public:
    SSEChannel(SSEConfig*, string);
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
    string header_data;
    long num_broadcasted_events;
    map<string, string> request_headers;
    ClientHandlerList::iterator curthread;
    SSEConfig *config;
    pthread_t _pingthread;
    deque<string> cache_keys;
    map<string, SSEEventPtr> cache_data;
    ClientHandlerList clientpool;
    char evs_preamble_data[2050];

    void AddResponseHeader(const string& header, const string& val);
    void CommitHeaderData();
    void InitializeThreads();
    void CleanupThreads();
    void Ping();
    static void* PingThread(void*);
};

typedef boost::shared_ptr<SSEChannel> SSEChannelPtr;

#endif
