#ifndef SSECLIENTWORKER_H
#define SSECLIENTWORKER_H

#include <memory>
#include <vector>
#include <mutex>
#include "SSEWorker.h"
#include "SSEServer.h"

typedef std::unordered_map<unsigned int, std::shared_ptr<SSEClient> > ClientList;

class SSEClientWorker : public SSEWorker {
  public:
    SSEClientWorker(SSEServer* server);
    ~SSEClientWorker();

  private:
    SSEServer* _server;
    int        _epoll_fd;

    ClientList _client_list;
    std::mutex _client_list_mutex;

    void _newClient(int fd, struct sockaddr_in* csin);
    void _acceptClient();
    void _read(std::shared_ptr<SSEClient> client, ClientList::iterator it);

  protected:
    void ThreadMain();
};

#endif
