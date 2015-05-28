#include <glog/logging.h>
#include <sys/epoll.h>
#include <errno.h>
#include "SSEClientHandler.h"

using namespace std;

SSEClientHandler::SSEClientHandler() {
  DLOG(INFO) << "SSEClientHandler constructor called.";

  epoll_fd = epoll_create1(0);
  LOG_IF(FATAL, epoll_fd == -1) << "epoll_create1 failed.";
}

SSEClientHandler::~SSEClientHandler() {
  DLOG(INFO) << "SSEClientHandler destructor called.";
}

void *SSEClientHandler::ThreadMain(void *pThis) {
  SSEClientHandler *pt = static_cast<SSEClientHandler*>(pThis);
  pt->ThreadMainFunc();
  return NULL;
}

void SSEClientHandler::ThreadMainFunc() {

}

void SSEClientHandler::AddClient(int fd) {
  int ret;
  struct epoll_event ev;

  ev.data.fd = fd;
  ev.events  = EPOLLIN | EPOLLET;
  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void SSEClientHandler::Broadcast(string msg) {

}


