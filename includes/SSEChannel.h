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
#include "SSEConfig.h"
#include "AMQPConsumer.h"
#include "SSEClientHandler.h"
#include "SSEClient.h"
#include "SSEEvent.h"
#include "HTTPRequest.h"

using namespace std;

typedef vector<SSEClientHandler*> ClientHandlerList;

class SSEChannel {
  public:
    SSEChannel(SSEConfig*, string);
    ~SSEChannel();
    string GetId();
    void AddClient(SSEClient* client, HTTPRequest* req);
    void Broadcast(const string& data);
    void BroadcastEvent(SSEEvent* event);
    void CacheEvent(SSEEvent* event);
    SSEEvent* GetEvent(const string& id);
    void SendEventsSince(SSEClient* client, string lastId);

  private:
    string id;
    string header_data;
    map<string, string> request_headers;
    ClientHandlerList::iterator curthread;
    SSEConfig *config;
    pthread_t _pingthread;
    deque<string> cache_keys;
    map<string, SSEEvent*> cache_data;
    ClientHandlerList clientpool;
    char evs_preamble_data[2050];

    void AddResponseHeader(const string& header, const string& val);
    void CommitHeaderData();
    void InitializeThreads();
    void CleanupThreads();
    void Ping();
    static void* PingThread(void*);
};

#endif
