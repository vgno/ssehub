#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>

using namespace std;

// Forward declarations.
class SSEClient;

typedef vector<SSEClient*> SSEClientPtrList;

class SSEClientHandler {
  public:
    SSEClientHandler(int);
    ~SSEClientHandler();

    bool AddClient(class SSEClient* client);
    bool RemoveClient(SSEClient* client);
    void Broadcast(const string& msg);
    long GetNumClients();

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
