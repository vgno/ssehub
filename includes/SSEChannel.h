#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
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

using namespace std;

struct SSEvent {
  long id;
  string data;
};

typedef vector<SSEClientHandler*> ClientHandlerList;

class SSEChannel {
  public:
    SSEChannel(SSEConfig*, string);
    ~SSEChannel();
    string GetId();
    void AddClient(SSEClient* client);
    void Broadcast(const string& data);
    void BroadcastEvent(SSEvent);

  private:
    string id;
    ClientHandlerList::iterator curthread;
    SSEConfig *config;
    pthread_t _pingthread;
    vector<SSEvent> event_history;
    ClientHandlerList clientpool;

    void InitializeThreads();
    void CleanupThreads();
    void Ping();
    static void* PingThread(void*);
};

#endif
