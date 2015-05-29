#include "SSEClient.h"
#include <glog/logging.h>

SSEClient::SSEClient(int fd) {
  uuid = boost::uuids::random_generator()();
  DLOG(INFO) << "Initialized client id: " << uuid;
  this->fd = fd;

  ev.data.fd = fd;
  ev.events  = EPOLLRDHUP | EPOLLHUP | EPOLLERR; // Detect disconnects and errors.
  ev.data.ptr = this;
}

struct epoll_event *SSEClient::GetEvent() {
  return &ev;
}

SSEClient::~SSEClient() {
  DLOG(INFO) << "Destructor called for client id: " << uuid;
  close(fd);
}
