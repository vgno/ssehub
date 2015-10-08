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
#include <vector>
#include <string>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include "SSEEvent.h"
#include "SSEStatsHandler.h"
#define MAXEVENTS 1024

extern int stop;

// Forward declarations.
class SSEConfig;
class SSEChannel;
class SSEInputSource;
class HTTPRequest;

typedef std::vector<boost::shared_ptr<SSEChannel> > SSEChannelList;

class SSEServer {
  public:
    SSEServer(SSEConfig* config);
    ~SSEServer();

    void Run();
    const SSEChannelList& GetChannelList();
    SSEConfig* GetConfig();
    bool Broadcast(SSEEvent& event);

  private:
    SSEConfig *_config;
    SSEChannelList _channels;
    boost::shared_ptr<SSEInputSource> _datasource;
    SSEStatsHandler stats;
    boost::thread _routerthread;
    int _serversocket;
    int _efd;
    struct sockaddr_in _sin;

    void InitSocket();
    void AcceptLoop();
    void ClientRouterLoop();
    void PostHandler(SSEClient* client, HTTPRequest* req);
    void InitChannels();
    SSEChannel* GetChannel(const std::string id, bool create);
};

#endif
