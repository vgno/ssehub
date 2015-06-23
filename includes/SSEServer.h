#ifndef SERVER_H

#define SERVER_H

#include <glog/logging.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>
#include <boost/shared_ptr.hpp>
#include "SSEStatsHandler.h"
#include "AMQPConsumer.h"

#define MAXEVENTS 1024

extern int stop;

// Forward declarations.
class SSEConfig;
class SSEChannel;

typedef vector<boost::shared_ptr<SSEChannel> > SSEChannelList;

class SSEServer {
  public:
    SSEServer(SSEConfig* config);
    ~SSEServer();
    
    void Run();
    static void *RouterThreadMain(void *);
    const SSEChannelList& GetChannelList();

  private:
    SSEConfig *config;
    SSEChannelList channels;
    AMQPConsumer amqp;
    SSEStatsHandler stats;
    int serverSocket;
    int efd;
    struct sockaddr_in sin;
    pthread_t routerThread;
    
    void InitSocket();
    void AcceptLoop();
    void ClientRouterLoop();
    static void AmqpCallbackWrapper(void *, const string, const string);
    void AmqpCallback(string, string);
    void InitChannels();
    SSEChannel* GetChannel(const string id);
};

#endif
