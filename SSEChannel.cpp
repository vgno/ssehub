#include "SSEChannel.h"
#include <iostream>

using namespace std;

SSEChannel::SSEChannel(string id) {
  this->id = id;
  DLOG(INFO) << "Initializing channel " << id;
} 

SSEChannel::~SSEChannel() {
  DLOG(INFO) << "SSEChannel destructor called.";
}

string SSEChannel::getID() {
  return id;
}

void SSEChannel::addClient(int fd) {
  LOG(INFO) << "Adding client to channel " << getID();
  close(fd);
}

void SSEChannel::broadcast(SSEvent event) {
  LOG(INFO) << "Channel " << id << ": " << "[" << event.id << "] " << event.data;
}

