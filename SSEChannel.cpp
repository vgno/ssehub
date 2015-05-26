#include "SSEChannel.h"
#include <iostream>

using namespace std;

SSEChannel::SSEChannel(SSEChannelConfig conf) {
  this->config = conf;
  DLOG(INFO) << "Initializing channel " << config.ID;

  amqp.start(config.amqpHost, config.amqpPort, config.amqpUser, 
    config.amqpPassword, config.amqpQueue, SSEChannel::amqpCallbackWrapper, this);
} 

SSEChannel::~SSEChannel() {
  DLOG(INFO) << "SSEChannel destructor called.";
}

void SSEChannel::amqpCallbackWrapper(void* pThis, const string msg) {
  SSEChannel* pt = static_cast<SSEChannel *>(pThis);

  pt->amqpCallback(msg);
}

void SSEChannel::amqpCallback(string msg) {
  LOG(INFO) << config.amqpQueue << ": " << msg;
}

string SSEChannel::getID() {
  return config.ID;
}

void SSEChannel::addClient(int fd) {
  LOG(INFO) << "Adding client to channel " << getID();
  close(fd);
}

