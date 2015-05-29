#include <glog/logging.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "SSEClientHandler.h"
#include "SSEClient.h"

using namespace std;

SSEClientHandler::SSEClientHandler(int tid) {
  DLOG(INFO) << "SSEClientHandler constructor called " << "id: " << tid;
  id = tid;
  num_clients = 0;

  epoll_fd = epoll_create1(0);
  LOG_IF(FATAL, epoll_fd == -1) << "epoll_create1 failed.";

  pthread_create(&_thread, NULL, &SSEClientHandler::ThreadMain, this); 
}

SSEClientHandler::~SSEClientHandler() {
  DLOG(INFO) << "SSEClientHandler destructor called for " << "id: " << id;
  StopThread();
}

void SSEClientHandler::StopThread() {
  pthread_cancel(_thread);
}

void *SSEClientHandler::ThreadMain(void *pThis) {
  SSEClientHandler *pt = static_cast<SSEClientHandler*>(pThis);
  pt->ThreadMainFunc();
  return NULL;
}

/*
* Listen for disconnect events and destroy client object for every client that disconnects.
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
        delete((SSEClient*)t_events[i].data.ptr);
        num_clients--;
      } else if (t_events[i].events & EPOLLERR) {
        DLOG(INFO) << "Thread " << id << ": Error.";
        delete((SSEClient*)t_events[i].data.ptr);
        num_clients--;
      }
    }
  }
}

void SSEClientHandler::AddClient(int fd) {
  int ret;

  SSEClient *client = new SSEClient(fd);
  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, client->GetEvent());
  DLOG_IF(ERROR, ret == -1) << "Failed to add client to epoll event list.";

  num_clients++;

  LOG(INFO) << "Client added to thread id: " << id;
  //close(fd);
}

void SSEClientHandler::Broadcast(string msg) {

}


