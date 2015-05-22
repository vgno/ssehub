#include "SSEServer.h"
#include <iostream>

using namespace std;

SSEServer::SSEServer(SSEConfig *config) {
  SSEChannelConfigList::iterator it;
  SSEChannelConfigList chanList = config->getChannels();

  for (it = chanList.begin(); it != chanList.end(); it++) {
    channels.push_back(new SSEChannel(*it));
  }

  this->config = config;
}

SSEServer::~SSEServer() {
  vector<SSEChannel*>::iterator it;

  DLOG(INFO) << "SSEServer destructor called.";

  for (it = channels.begin(); it != channels.end(); it++) {
    free(*it);
  }
}

void SSEServer::run() {
  while(1) {
    sleep(1);
  }
}