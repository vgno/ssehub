#ifndef SSECLIENTWORKER_H
#define SSECLIENTWORKER_H

#include <memory>
#include <vector>
#include <mutex>
#include "SSEWorker.h"
#include "SSEServer.h"

typedef std::vector<std::shared_ptr<SSEClient> > ClientList;

class SSEClientWorker : public SSEWorker {
  public:
    SSEClientWorker(SSEServer* server);
    ~SSEClientWorker();

    void NewClient(int fd, struct sockaddr_in* csin);

  private:
    SSEServer* _server;
    int        _epoll_fd;

    ClientList _client_list;
    std::mutex _client_list_mutex;

  protected:
    void ThreadMain();
};

#endif
