#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
#include <string>
#include <glog/logging.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include "SSEConfig.h"
#include "AMQPConsumer.h"
#include "SSEClientHandler.h"

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
    void Broadcast(SSEvent);
    void AddClient(int);

  private:
    string id;
    ClientHandlerList::iterator curthread;
    SSEConfig *config;
    vector<SSEvent> event_history;
    ClientHandlerList clientpool;

    void InitializeThreads();
    void CleanupThreads();
};

#endif
