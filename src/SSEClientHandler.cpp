#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <climits>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "Common.h"
#include "SSEClientHandler.h"
#include "SSEClient.h"

extern int stop;

using namespace std;

/**
  Constructor.
  @param tid unique ID to identify thread.
*/
SSEClientHandler::SSEClientHandler(int tid) {
  DLOG(INFO) << "SSEClientHandler constructor called " << "id: " << tid;
  _id = tid;

  _processorthread = boost::thread(boost::bind(&SSEClientHandler::ProcessQueue, this));
}

/**
  Destructor.
*/
SSEClientHandler::~SSEClientHandler() {
  DLOG(INFO) << "SSEClientHandler destructor called for " << "id: " << _id;
  pthread_cancel(_processorthread.native_handle());
}

/**
  Add client to pool.
  @param client SSEClient pointer.
*/
void SSEClientHandler::AddClient(SSEClientPtr& client) {
  _clientlist.push_back(SSEWeakClientPtr(client));
  DLOG(INFO) << "Client added to thread id: " << _id;
}

/**
  Broadcast message to all clients connected to this clienthandler.
  @param msg String to broadcast.
*/
void SSEClientHandler::Broadcast(const string msg) {
  _msgqueue.Push(msg);
}

void SSEClientHandler::ProcessQueue() {
  while(!stop) {
    std::string msg;
    _msgqueue.WaitPop(msg);

   for (SSEWeakClientPtrList::iterator it = _clientlist.begin(); it != _clientlist.end(); it++) {
     SSEWeakClientPtr w_client = static_cast<SSEWeakClientPtr&>(*it); // Should we use a reference or not here ?.
     SSEClientPtr client;

     if (!(client = w_client.lock())) {
       DLOG(INFO) << "Removing disconnected client from clienthandler.";
       it = _clientlist.erase(it);
       continue;
     }

     client->Send(msg);
   }
  }
}
