#ifndef SERVER_H
#define SERVER_H
#include "SSEConfig.h"
#include "SSEChannel.h"

class SSEServer {
  public:
    SSEServer(SSEConfig*);
    ~SSEServer();
    void run();
    
  private:
    SSEConfig *config;
    vector<SSEChannel*> channels;
};

#endif