#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>
#include "SSEClient.h"

using namespace std;

class SSEClientHandler {
  public:
    SSEClientHandler(int);
    ~SSEClientHandler();

    void AddClient(SSEClient* client);
    bool RemoveClient(SSEClient* client);
    void Broadcast(const string& msg);

  private:
    int id;
    int epoll_fd;
    long num_clients;
    pthread_t _thread;
    SSEClientPtrList client_list;

    static void *ThreadMain(void*);  
    void ThreadMainFunc();
};


#endif
