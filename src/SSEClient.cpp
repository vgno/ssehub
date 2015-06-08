#include "SSEClient.h"
#include <glog/logging.h>
#include <sys/socket.h>

SSEClient::SSEClient(int fd) {
  uuid = boost::uuids::random_generator()();
  DLOG(INFO) << "Initialized client id: " << uuid;
  this->fd = fd;
}

int SSEClient::Send(const string &data) {
  return send(fd, data.c_str(), data.length(), 0);
}

SSEClient::~SSEClient() {
  DLOG(INFO) << "Destructor called for client id: " << uuid;
  close(fd);
}

int SSEClient::Getfd() {
  return fd;
}

boost::uuids::uuid SSEClient::GetId() {
  return uuid;
}
