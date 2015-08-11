#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "ConcurrentQueue.h"

using namespace std;

// Forward declarations.
class SSEClient;

typedef boost::shared_ptr<SSEClient> SSEClientPtr;
typedef list<SSEClientPtr> SSEClientPtrList;

class SSEClientHandler {
  public:
    SSEClientHandler(int);
    ~SSEClientHandler();
    void AddClient(SSEClient* client);
    void Broadcast(const string msg);
    size_t GetNumClients();

  private:
    int _id;
    SSEClientPtrList _clientlist;
    boost::thread _processorthread;
    ConcurrentQueue<std::string> _msgqueue;

    void ProcessQueue();
};

#endif
