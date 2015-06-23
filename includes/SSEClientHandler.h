#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>
#include <list>
#include <boost/shared_ptr.hpp>

using namespace std;

// Forward declarations.
class SSEClient;

typedef boost::shared_ptr<SSEClient> SSEClientPtr;
typedef list<SSEClientPtr> SSEClientPtrList;

struct SSEClientHandlerStats {
  long num_connects;
  long num_disconnects;
  long num_errors;
  long num_clients;
};


class SSEClientHandler {
  public:
    SSEClientHandler(int);
    ~SSEClientHandler();
    bool AddClient(class SSEClient* client);
    void Broadcast(const string msg);
    const SSEClientHandlerStats& GetStats();

  private:
    int id;
    int epoll_fd;
    long num_clients;
    pthread_t cleanup_thread;
    pthread_mutex_t _bcast_mutex;
    SSEClientPtrList client_list;
    SSEClientHandlerStats _stats;

    static void* CleanupMain(void* pThis);
    void CleanupMainFunc();
    static void* AsyncBroadcastFunc(void* mp);
};

#endif
