#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>
#include "SSEClient.h"

using namespace std;

typedef vector<SSEClientPtr*> SSEClientList;

class SSEClientHandler {
  public:
    SSEClientHandler(int);
    ~SSEClientHandler();

    void AddClient(int fd);
    void Broadcast(string msg);
    void StopThread();

  private:
    int id;
    int epoll_fd;
    long num_clients;
    pthread_t _thread;
    pthread_cond_t broadcast_cond;
    pthread_mutex_t broadcast_mutex;
    string broadcast_buffer;
    //SSEClientList client_list;

    static void *ThreadMain(void*);  
    void ThreadMainFunc();
};


#endif
