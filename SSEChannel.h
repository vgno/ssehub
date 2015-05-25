#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
#include <string>
#include <glog/logging.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include "SSEConfig.h"
#include "SSEClient.h"
#include "AMQPConsumer.h"

using namespace std;

typedef struct {
  long timestamp;
  string body;
} SSEvent;

class SSEChannel {
  public:
    SSEChannel(SSEChannelConfig);
    ~SSEChannel();
    string getID();
    void broadcast(SSEvent *event);
    void consume();
    static void amqpCallbackWrapper(void *, const string);
    void amqpCallback(string);
    void addClient(int);

  private:
    vector<SSEClient> clientList;
    vector<SSEvent> eventHistory;
    SSEChannelConfig config;
    AMQPConsumer amqp;

};

#endif
