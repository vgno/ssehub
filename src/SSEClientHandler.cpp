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
  StopThread();
}

/**
  Stop the thread listening for client disconnectds.
*/
void SSEClientHandler::StopThread() {
  pthread_cancel(_thread);
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
  int n, i;
  struct epoll_event *t_events, t_event;

  t_events = (epoll_event*)calloc(1024, sizeof(t_event));

  while(1) {
    n = epoll_wait(epoll_fd, t_events, 1024, -1);

    for (i = 0; i < n; i++) {
      if ((t_events[i].events & EPOLLHUP) || (t_events[i].events & EPOLLRDHUP)) {
        DLOG(INFO) << "Thread " << id << ": Client disconnected.";
        RemoveClient((SSEClient*)t_events[i].data.ptr);
      } else if (t_events[i].events & EPOLLERR) {
        // If an error occours on a client socket, just drop the connection.
        DLOG(INFO) << "Thread " << id << ": Error on client socket: " << strerror(errno);
        RemoveClient((SSEClient*)t_events[i].data.ptr);
      }
    }
  }
}

/**
  Add client to pool.
  @param client SSEClient pointer.
*/
void SSEClientHandler::AddClient(SSEClient* client) {
  int ret;
  struct epoll_event ev;

  client_list.push_back(client);

  ev.data.fd  = client->Getfd();
  ev.events   = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLET;
  ev.data.ptr = client;

  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->Getfd(), &ev);
  DLOG_IF(ERROR, ret == -1) << "Failed to add client to epoll event list.";
 
  num_clients++;

  DLOG(INFO) << "Client added to thread id: " << id << " Numclients: " << num_clients;
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
  delete(client);
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
