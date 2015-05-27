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

using namespace std;

struct SSEvent {
  long id;
  string data;
};

class SSEChannel {
  public:
    SSEChannel(string);
    ~SSEChannel();
    string getID();
    void broadcast(SSEvent);
    void addClient(int);

  private:
    string id;
    vector<SSEvent> eventHistory;
};

#endif
