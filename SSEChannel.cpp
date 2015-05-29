#include "SSEChannel.h"
#include <iostream>

using namespace std;

SSEChannel::SSEChannel(SSEConfig* conf, string id) {
  this->id = id;
  this->config = conf;
  DLOG(INFO) << "Initializing channel " << id;
  DLOG(INFO) << "Threads per channel: " << config->getServer().threadsPerChannel;
  InitializeThreads();
}

SSEChannel::~SSEChannel() {
  DLOG(INFO) << "SSEChannel destructor called.";
  CleanupThreads();
}

void SSEChannel::InitializeThreads() {
  int i;

  for (i = 0; i < config->getServer().threadsPerChannel; i++) {
    clientpool.push_back(new SSEClientHandler(i));
  }

  curthread = clientpool.begin();
}

void SSEChannel::CleanupThreads() {
  ClientHandlerList::iterator it;

  for (it = clientpool.begin(); it != clientpool.end(); it++) { 
   delete(*it);
  }  
}

string SSEChannel::GetId() {
  return id;
}

void SSEChannel::AddClient(int fd) {
  LOG(INFO) << "Adding client to channel " << GetId();
  (*curthread)->AddClient(fd); 

  *curthread++;
  if (curthread == clientpool.end()) curthread = clientpool.begin();
}

void SSEChannel::Broadcast(SSEvent event) {
  LOG(INFO) << "Channel " << id << ": " << "[" << event.id << "] " << event.data;
}

