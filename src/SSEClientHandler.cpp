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
  _connected_clients = 0;

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
void SSEClientHandler::AddClient(SSEClient* client) {
  _clientlist.push_back(SSEClientPtr(client));
  _connected_clients++;
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

    boost::mutex::scoped_lock lock(_clientlist_lock);

    unsigned int i = 0;
    for (SSEClientPtrList::iterator it = _clientlist.begin(); it != _clientlist.end();) {
      SSEClientPtr client = static_cast<SSEClientPtr&>(*it);

      if (client->IsDead()) {
        DLOG(INFO) << "Removing disconnected client from clienthandler.";
        it = _clientlist.erase(it);
        _connected_clients--;
        continue;
      }

      client->Send(msg);

      it++;
      i++;
    }
    DLOG(INFO) << "Clienthandler " << _id << " broadcast to " << i << " clients.";
  }
}

/**
  Returns number of clients connected to this clienthandler thread.
*/
size_t SSEClientHandler::GetNumClients() {
  return _connected_clients;
}
