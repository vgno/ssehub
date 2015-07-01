#ifndef SSECLIENTHANDLER_H
#define SSECLIENTHANDLER_H

#include <string>
#include <pthread.h>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread.hpp>
#include "ConcurrentQueue.h"

using namespace std;

// Forward declarations.
class SSEClient;

typedef boost::shared_ptr<SSEClient> SSEClientPtr;
typedef boost::weak_ptr<SSEClient> SSEWeakClientPtr;
typedef list<SSEWeakClientPtr> SSEWeakClientPtrList;

class SSEClientHandler {
  public:
    SSEClientHandler(int);
    ~SSEClientHandler();
    void AddClient(SSEClientPtr& client);
    void Broadcast(const string msg);

  private:
    int _id;
    SSEWeakClientPtrList _clientlist;
    boost::thread _processorthread;
    ConcurrentQueue<std::string> _msgqueue;

    void ProcessQueue();
};

#endif
