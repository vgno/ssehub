#include <glog/logging.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "SSEClient.h"

/**
 Constructor.
 @param fd Client socket file descriptor.
 @param csin Client sockaddr_in structure.
*/
SSEClient::SSEClient(int fd, struct sockaddr_in* csin) {
  this->fd = fd;
  memcpy(&_csin, csin, sizeof(struct sockaddr_in));
  uuid = boost::uuids::random_generator()();
  DLOG(INFO) << "Initialized client id: " << uuid << " ip: " << GetIP();
}

/**
  Destructor.
*/
void SSEClient::Destroy() {
  delete(this);
}

/**
 Sends data to client.
 @param data String buffer to send.
*/
int SSEClient::Send(const string &data) {
  return send(fd, data.c_str(), data.length(), 0);
}

/**
 Read data from client.
 @param buf Pointer to buffer where data should be read into.
 @param len Bytes to read.
*/
size_t SSEClient::Read(void* buf, int len) {
  return read(fd, buf, len);
}

/**
  Destructor.
*/
SSEClient::~SSEClient() {
  DLOG(INFO) << "Destructor called for client id: " << uuid << " ip: " << GetIP();
  close(fd);
}

/**
 Returns the client file descriptor.
*/
int SSEClient::Getfd() {
  return fd;
}

/**
  Returns the client's ip address as a string.
*/
const string SSEClient::GetIP() {
  char ip[32];
  inet_ntop(AF_INET, &_csin.sin_addr, (char*)&ip, 32);

  return ip;
}

/**
  Get a unique identifier of the client.
*/
boost::uuids::uuid SSEClient::GetId() {
  return uuid;
}
