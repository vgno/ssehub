#ifndef SSEACCEPTORWORKER_H
#define SSEACCEPTORWORKER_H

#include "SSEWorker.h"
#include "SSEServer.h"

class SSEAcceptorWorker : public SSEWorker {
  public:
    SSEAcceptorWorker(SSEServer* server);
    ~SSEAcceptorWorker();

  private:
    SSEServer* _server;
    int        _epoll_fd;

  protected:

    void ThreadMain();
};

#endif
