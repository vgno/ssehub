#include "Common.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <boost/shared_ptr.hpp>
#include "SSEClient.h"
#include "HTTPRequest.h"

/**
 Constructor.
 @param fd Client socket file descriptor.
 @param csin Client sockaddr_in structure.
*/
SSEClient::SSEClient(int fd, struct sockaddr_in* csin) {
  _fd = fd;
  _dead = false;
  memcpy(&_csin, csin, sizeof(struct sockaddr_in));
  DLOG(INFO) << "Initialized client with IP: " << GetIP();

  m_httpReq = boost::shared_ptr<HTTPRequest>(new HTTPRequest());

  // Set TCP_NODELAY on socket.
  int flag = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
  
  // Set KEEPALIVE on socket.
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int));

  // If we have TCP_USER_TIMEOUT set it to 10 seconds.
  #ifdef TCP_USER_TIMEOUT
  int timeout = 10000;
  setsockopt (fd, SOL_TCP, TCP_USER_TIMEOUT, (char*) &timeout, sizeof (timeout));
  #endif
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
  boost::mutex::scoped_lock lock(_writelock);
  return send(_fd, data.c_str(), data.length(), MSG_NOSIGNAL);
}

/**
 Read data from client.
 @param buf Pointer to buffer where data should be read into.
 @param len Bytes to read.
*/
size_t SSEClient::Read(void* buf, int len) {
  return read(_fd, buf, len);
}

/**
  Destructor.
*/
SSEClient::~SSEClient() {
  DLOG(INFO) << "Destructor called for client with IP: " << GetIP();
  if (!_dead) close(_fd);
}

/**
 Returns the client file descriptor.
*/
int SSEClient::Getfd() {
  return _fd;
}

/**
  Returns the client's ip address as a string.
*/
const string SSEClient::GetIP() {
  char ip[32];
  inet_ntop(AF_INET, &_csin.sin_addr, (char*)&ip, 32);

  return ip;
}

uint32_t SSEClient::GetSockAddr() {
  return _csin.sin_addr.s_addr;
}

HTTPRequest* SSEClient::GetHttpReq() {
  return m_httpReq.get();
}

void SSEClient::DeleteHttpReq() {
  m_httpReq.reset();
}

void SSEClient::MarkAsDead() {
  _dead = true;
  close(_fd);
}

bool SSEClient::IsDead() {
  return _dead;
}
