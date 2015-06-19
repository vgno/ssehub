#include "Common.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <boost/shared_ptr.hpp>
#include "SSEClient.h"
#include "HTTPRequest.h"

/**
 Constructor.
 @param fd Client socket file descriptor.
 @param csin Client sockaddr_in structure.
*/
SSEClient::SSEClient(int fd, struct sockaddr_in* csin) {
  this->fd = fd;
  memcpy(&_csin, csin, sizeof(struct sockaddr_in));
  DLOG(INFO) << "Initialized client with IP: " << GetIP();
  
  m_httpReq = boost::shared_ptr<HTTPRequest>(new HTTPRequest());
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
  DLOG(INFO) << "Destructor called for client with IP: " << GetIP();
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

HTTPRequest* SSEClient::GetHttpReq() {
  return m_httpReq.get(); 
}

void SSEClient::DeleteHttpReq() {
  m_httpReq.reset();
}
