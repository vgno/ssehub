#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <climits>
#include <boost/shared_ptr.hpp>
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
  id = tid;

  _stats.num_clients     = 0;
  _stats.num_disconnects = 0;
  _stats.num_connects    = 0;
  _stats.num_errors      = 0;

  epoll_fd = epoll_create1(0);
  LOG_IF(FATAL, epoll_fd == -1) << "epoll_create1 failed.";

  pthread_mutex_init(&_bcast_mutex, NULL);
  pthread_create(&cleanup_thread, NULL, &SSEClientHandler::CleanupMain, this);
}

/**
  Destructor.
*/
SSEClientHandler::~SSEClientHandler() {
  DLOG(INFO) << "SSEClientHandler destructor called for " << "id: " << id;
  pthread_cancel(cleanup_thread);
  close(epoll_fd);
}

/**
  Wrapper function for CleanupMainFunc to satisfy the function-type pthread expects.
*/
void *SSEClientHandler::CleanupMain(void *pThis) {
  SSEClientHandler *pt = static_cast<SSEClientHandler*>(pThis);
  pt->CleanupMainFunc();
  return NULL;
}

/**
  Listen for disconnect events and destroy client object for every client disconnect.
*/
void SSEClientHandler::CleanupMainFunc() {
  boost::shared_ptr<struct epoll_event[]> t_events(new struct epoll_event[1024]);
  
  while(!stop) {
    int n = epoll_wait(epoll_fd, t_events.get(), 1024, -1);

    for (int i = 0; i < n; i++) {
      SSEClient* client;
      client = static_cast<SSEClient*>(t_events[i].data.ptr);

      if ((t_events[i].events & EPOLLHUP) || (t_events[i].events & EPOLLRDHUP)) {
        DLOG(INFO) << "Thread " << id << ": Client disconnected.";
        client->MarkAsDead();
        INC_LONG(_stats.num_disconnects);
      } else if (t_events[i].events & EPOLLERR) {
        // If an error occours on a client socket, just drop the connection.
        DLOG(INFO) << "Thread " << id << ": Error on client socket: " << strerror(errno);
        client->MarkAsDead();
        INC_LONG(_stats.num_errors);
      }
    }
  }
}

/**
  Add client to pool.
  @param client SSEClient pointer.
*/
bool SSEClientHandler::AddClient(SSEClient* client) {
  int ret;
  struct epoll_event ev;

  ev.events   = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLET;
  ev.data.ptr = client;
  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->Getfd(), &ev);

  if (ret == -1) {
    DLOG(ERROR) << "Failed to add client " << client->GetIP() << " to epoll event list.";
    client->Destroy();
    return false;
  }

  client_list.push_back(SSEClientPtr(client));
  _stats.num_clients++;
  INC_LONG(_stats.num_connects);

  DLOG(INFO) << "Client added to thread id: " << id << " Numclients: " << _stats.num_clients;

  return true;
}


/**
  Broadcast message to all clients connected to this clienthandler.
  Also handles the removal of disconnected client objects.
  @param msg String to broadcast.
*/
void SSEClientHandler::Broadcast(const string msg) {
  SSEClientPtrList::iterator it;

  pthread_mutex_lock(&_bcast_mutex);
  for (it = client_list.begin(); it != client_list.end(); it++) {
    SSEClientPtr& client = static_cast<SSEClientPtr&>(*it);

    // Check that the client object is valid - it should always be,
    // but better safe than sorry.
    if (!client) {
      LOG(ERROR) << "Invalid SSEClient pointer found, removing.";
      it = client_list.erase(it);
      _stats.num_clients--;
      continue;
    }

    // If client is marked as dead, remove it.
    if (client->IsDead()) {
      DLOG(INFO) << "Client " << client->GetIP() << " is marked as dead, removing.";
      it = client_list.erase(it);
      _stats.num_clients--;
      continue;
    }

    client->Send(msg);
  }

  pthread_mutex_unlock(&_bcast_mutex);
}

const SSEClientHandlerStats& SSEClientHandler::GetStats() {
  return _stats;
}
