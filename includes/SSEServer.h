#ifndef SERVER_H
#define SERVER_H
#include "SSEConfig.h"
#include "SSEChannel.h"
#include "AMQPConsumer.h"
#include <glog/logging.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>

#define MAXEVENTS 1024

extern int stop;

typedef vector<SSEChannel*> SSEChannelList;

class SSEServer {
  public:
    SSEServer(SSEConfig*);
    ~SSEServer();
    
    void Run();
    static void *RouterThreadMain(void *);
    string GetUri(const char *, int len);

  private:
    SSEConfig *config;
    SSEChannelList channels;
    AMQPConsumer amqp;
    int serverSocket;
    int efd;
    struct epoll_event *eventList;
    struct sockaddr_in sin;
    pthread_t routerThread;
    
    void InitSocket();
    void AcceptLoop();
    void ClientRouterLoop();
    static void AmqpCallbackWrapper(void *, const string, const string);
    void AmqpCallback(string, string);
    SSEChannel* GetChannel(const string id);
};

#endif
