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

string SSEChannel::GetId() {
  return id;
}

void SSEChannel::AddClient(int fd) {
  LOG(INFO) << "Adding client to channel " << GetId();
  close(fd);
}

void SSEChannel::Broadcast(SSEvent event) {
  LOG(INFO) << "Channel " << id << ": " << "[" << event.id << "] " << event.data;
}

