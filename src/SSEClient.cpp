#include <glog/logging.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "SSEClient.h"

SSEClient::SSEClient(int fd, struct sockaddr_in* csin) {
  this->fd = fd;
  memcpy(&_csin, csin, sizeof(struct sockaddr_in));
  uuid = boost::uuids::random_generator()();
  DLOG(INFO) << "Initialized client id: " << uuid << " ip: " << GetIP();
}

int SSEClient::Send(const string &data) {
  return send(fd, data.c_str(), data.length(), 0);
}

size_t SSEClient::Read(void* buf, int len) {
  return read(fd, buf, len);
}

SSEClient::~SSEClient() {
  DLOG(INFO) << "Destructor called for client id: " << uuid << " ip: " << GetIP();
  close(fd);
}

int SSEClient::Getfd() {
  return fd;
}

const string SSEClient::GetIP() {
  char ip[32];
  inet_ntop(AF_INET, &_csin.sin_addr, (char*)&ip, 32);

  return ip;
}

void SSEClient::Destroy() {
  delete(this);
}

boost::uuids::uuid SSEClient::GetId() {
  return uuid;
}
