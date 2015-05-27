#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>

using namespace std;

class SSEClientHandler {
  public:
    SSEClientHandler();
    ~SSEClientHandler();

    void AddClient(int fd);
    void Broadcast(string msg);

  private:
    int epoll_fd;
    pthread_t _thread;
    pthread_cond_t broadcast_cond;
    pthread_mutex_t broadcast_mutex;
    string broadcast_buffer;

    static void *ThreadMain(void*);  
    void ThreadMainFunc();
};

#endif
