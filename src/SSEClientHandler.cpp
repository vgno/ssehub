#include <glog/logging.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "SSEClientHandler.h"
#include "SSEClient.h"

using namespace std;

/**
  Constructor.
  @param tid unique ID to identify thread.
*/
SSEClientHandler::SSEClientHandler(int tid) {
  DLOG(INFO) << "SSEClientHandler constructor called " << "id: " << tid;
  id = tid;
  num_clients = 0;

  epoll_fd = epoll_create1(0);
  LOG_IF(FATAL, epoll_fd == -1) << "epoll_create1 failed.";

  pthread_create(&_thread, NULL, &SSEClientHandler::ThreadMain, this); 
}

/**
  Destructor.
*/
SSEClientHandler::~SSEClientHandler() {
  DLOG(INFO) << "SSEClientHandler destructor called for " << "id: " << id;
  pthread_cancel(_thread);
  close(epoll_fd);
}

/**
  Wrapper function for ThreadMainFunc to satisfy the function-type pthread expects.
*/
void *SSEClientHandler::ThreadMain(void *pThis) {
  SSEClientHandler *pt = static_cast<SSEClientHandler*>(pThis);
  pt->ThreadMainFunc();
  return NULL;
}

/**
  Listen for disconnect events and destroy client object for every client disconnect.
*/
void SSEClientHandler::ThreadMainFunc() {
  struct epoll_event *t_events, t_event;

  t_events = static_cast<epoll_event*>(calloc(1024, sizeof(t_event)));

  while(1) {
    int n = epoll_wait(epoll_fd, t_events, 1024, -1);

    for (int i = 0; i < n; i++) {
      SSEClient* client;
      client = static_cast<SSEClient*>(t_events[i].data.ptr);

      if ((t_events[i].events & EPOLLHUP) || (t_events[i].events & EPOLLRDHUP)) {
        DLOG(INFO) << "Thread " << id << ": Client disconnected.";
        RemoveClient(client);
      } else if (t_events[i].events & EPOLLERR) {
        // If an error occours on a client socket, just drop the connection.
        DLOG(INFO) << "Thread " << id << ": Error on client socket: " << strerror(errno);
        RemoveClient(client);
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

  client_list.push_back(client);
  num_clients++;

  DLOG(INFO) << "Client added to thread id: " << id << " Numclients: " << num_clients;

  return true;
}

/**
  Remove client from client list.
  TODO: Optimize.
  @param client pointer to SSEClient to remove.
*/
bool SSEClientHandler::RemoveClient(SSEClient* client) {
  SSEClientPtrList::iterator it;
  it = std::find(client_list.begin(), client_list.end(), client);

  if (it == client_list.end()) return false;

  client_list.erase(it);
  client->Destroy();
  num_clients--;

  return true;
}

/**
  Broadcast message to all clients connected to this clienthandler.
  @param msg String to broadcast.
*/
void SSEClientHandler::Broadcast(const string& msg) {
  SSEClientPtrList::iterator it;

  for (it = client_list.begin(); it != client_list.end(); it++) {
    (*it)->Send(msg);
  }
}

long SSEClientHandler::GetNumClients() {
  return num_clients;
}
