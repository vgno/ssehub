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
#include <mutex>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>
#include "SSEEvent.h"
#include "SSEStatsHandler.h"
#include "SSEWorker.h"
#include "SSEClientWorker.h"
#include "SSEAcceptorWorker.h"

extern int stop;

// Forward declarations.
class SSEConfig;
class SSEChannel;
class SSEInputSource;
class HTTPRequest;
class SSEClientWorker;
class SSEAcceptorWorker;
class SSEServer;

typedef std::vector<boost::shared_ptr<SSEChannel> > SSEChannelList;

class SSEServer {
  public:
    SSEServer(SSEConfig* config);
    ~SSEServer();

    void Run();
    int GetListeningSocket();
    const SSEChannelList& GetChannelList();
    SSEConfig* GetConfig();
    SSEClientWorker* GetClientWorker();
    bool IsAllowedToPublish(SSEClient* client, const struct ChannelConfig& chConf);
    bool Broadcast(SSEEvent& event);

  private:
    SSEConfig *_config;
    SSEChannelList _channels;
    boost::shared_ptr<SSEInputSource> _datasource;
    SSEStatsHandler stats;
    SSEWorkerGroup<SSEAcceptorWorker> _acceptors;
    SSEWorkerGroup<SSEClientWorker> _client_workers;
    int _serversocket;
    struct sockaddr_in _sin;

    void InitSocket();
    void PostHandler(SSEClient* client, HTTPRequest* req);
    void InitChannels();
    int  GetClientWorkerRR();
    SSEChannel* GetChannel(const std::string id, bool create=false);
};

#endif
