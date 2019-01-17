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
#include <boost/thread/mutex.hpp>
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

typedef struct {
  unsigned int  id;
  boost::thread thread;
  int           epoll_fd;
} worker_ctx_t;

typedef std::vector<boost::shared_ptr<worker_ctx_t> > WorkerThreadList;

class SSEServer {
  public:
    SSEServer(SSEConfig* config);
    ~SSEServer();

    void Run();
    const SSEChannelList& GetChannelList();
    SSEConfig* GetConfig();
    bool IsAllowedToPublish(SSEClient* client, const struct ChannelConfig& chConf);
    bool Broadcast(SSEEvent& event);

  private:
    SSEConfig *_config;
    SSEChannelList _channels;
    boost::shared_ptr<SSEInputSource> _datasource;
    SSEStatsHandler stats;
    WorkerThreadList _acceptWorkers;
    WorkerThreadList _clientWorkers;
    WorkerThreadList::iterator _curClientWorker;
    boost::mutex _clientWorkerLock;
    int _serversocket;
    struct sockaddr_in _sin;

    void InitSocket();
    void AcceptWorker(boost::shared_ptr<worker_ctx_t> ctx);
    void ClientWorker(boost::shared_ptr<worker_ctx_t> ctx);
    void PostHandler(SSEClient* client, HTTPRequest* req);
    void InitChannels();
    int  GetClientWorkerRR();
    SSEChannel* GetChannel(const std::string id, bool create=false);
};

#endif
