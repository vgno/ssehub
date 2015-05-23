#ifndef SERVER_H
#define SERVER_H
#include "SSEConfig.h"
#include "SSEChannel.h"
#include <glog/logging.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

class SSEServer {
  public:
    SSEServer(SSEConfig*);
    ~SSEServer();
    void run();
    static void *routerThreadMain(void *);
    
  private:
    SSEConfig *config;
    vector<SSEChannel*> channels;
    int serverSocket;
    struct sockaddr_in sin;
    pthread_t routerThread;

    void initSocket();
    void initChannels();
    void acceptLoop();
    void clientRouterLoop();
};

#endif