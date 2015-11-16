#include "Common.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include "SSEClient.h"
#include "HTTPRequest.h"
#include <sys/uio.h>

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

  int flag = 1;

  _isIdFiltered = false;
  _isEventFiltered = false;

  // Set KEEPALIVE on socket.
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int));

  // If we have TCP_USER_TIMEOUT set it to 10 seconds.
  #ifdef TCP_USER_TIMEOUT
  int timeout = 10000;
  setsockopt (fd, SOL_TCP, TCP_USER_TIMEOUT, (char*) &timeout, sizeof (timeout));
  #endif

  // Set TCP_NODELAY on socket.
  setsockopt(Getfd(), IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
}

/**
  Destructor.
*/
void SSEClient::Destroy() {
  delete(this);
}

/*
 Cut off @param bytes from our internal sendbuffer.
 @param bytes Number of bytes to cut off.
*/
size_t SSEClient::PruneSendBufferBytes(size_t bytes) {
  boost::mutex::scoped_lock lock(_sndBufLock);
  int bytes_used = 0;
  int i = 0;

  for (vector<string>::iterator it = _sndBuf.begin(); it != _sndBuf.end(); it++, i++) {
    if ((bytes_used + (*it).length()) > bytes) {
      int used_here = bytes-bytes_used;

      *it = (*it).substr(used_here, (*it).length()-used_here);

      if (i > 1) {
        _sndBuf.erase(_sndBuf.begin(), it);
      }

      break;
    }
    bytes_used += (*it).length();
  }

  return _sndBuf.size();
}

size_t SSEClient::PruneSendBufferItems(size_t items) {
  boost::mutex::scoped_lock lock(_sndBufLock);
  vector<string>::iterator it;

  if (items > _sndBuf.size()) {
    LOG(ERROR) << "PruneSendBufferItems " << "items: " << items << " _sndBuf.size(): " << _sndBuf.size();;
    _sndBuf.clear();
    return 0;
  }

  it  = _sndBuf.begin();
  it += items;

  _sndBuf.erase(_sndBuf.begin(), it);
  return _sndBuf.size();
}

int SSEClient::Flush() {
  int ret;
  
  if (_sndBuf.size() < 1) return 0;
  _sndBufLock.lock();

  // Use writev to write IOVEC_SIZE chunks of _sndBuf at one time.
  for (unsigned int i = 0; i <= (_sndBuf.size() / IOVEC_SIZE); i++) {
    struct iovec siov[IOVEC_SIZE];
    long siov_len = 0;
    int siov_cnt = 0;

    BOOST_FOREACH(const string& buf, _sndBuf) {
      if (siov_cnt == IOVEC_SIZE) break;
      siov[siov_cnt].iov_base  = (char*)buf.c_str();
      siov[siov_cnt].iov_len   = buf.length();
      siov_len                += buf.length();
      siov_cnt++;
    }

    _sndBufLock.unlock();

    ret = writev(_fd, siov, siov_cnt);

    if (ret == -1) {
      DLOG(INFO) << GetIP() << ": flush error: " << strerror(errno);
      break;
    }

    if (ret < siov_len) {
      // Remove data that was written from _sndBuf.
      PruneSendBufferBytes(ret);
      DLOG(INFO) << GetIP() << ": Could not write entire buffer, wrote " << ret << " of " << siov_len << " bytes.";
      break;
    }

    // Remove sent elements from _sndBuf.
    PruneSendBufferItems(siov_cnt);
  }

  return ret;
}

/**
 Sends data to client.
 @param data String buffer to send.
*/
int SSEClient::Send(const string &data, bool flush) {
  if (!isFilterAcceptable(data)) return 0;

  _sndBufLock.lock();
  _sndBuf.push_back(boost::move(data));
  _sndBufLock.unlock();

  if (flush) Flush();
  return _sndBuf.size();
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

const string SSEClient::GetSSEField(const string& data, const string& fieldName) {
  size_t field_pos = data.find(fieldName + ": ");
  size_t field_val_offset;
  string field_val;

  if (field_pos != string::npos) {
    // Add +2 to the offset to take account for ": ".
    field_val_offset = fieldName.length() + 2;

    size_t field_nl_pos = data.find_first_of('\n', field_pos);
    if (field_nl_pos != string::npos) {
      field_val = data.substr(field_pos+field_val_offset, field_nl_pos-field_val_offset);
    }
  }

  return field_val;
}

bool SSEClient::isSubscribed(const string key, SubscriptionType type) {
 BOOST_FOREACH(const SubscriptionElement& subscription, _subscriptions) {
    if (subscription.type == type && (subscription.key.compare(0, subscription.key.length(), key) == 0)) {
      return true;
    }
  }

 return false;
}

void SSEClient::Subscribe(const string key, SubscriptionType type) {
  SubscriptionElement subscription;
  subscription.key  = key;
  subscription.type = type;
  if (type == SUBSCRIPTION_ID) _isIdFiltered = true;
  if (type == SUBSCRIPTION_EVENT_TYPE) _isEventFiltered = true;
  _subscriptions.push_back(subscription);
}

bool SSEClient::isFilterAcceptable(const string& data) {
  // No filters defined.
  if (_subscriptions.size() < 1) return true;

  // Only filter payloads having the "data: " field set.
  if ((data.compare(0, 6, "data: ") != 0) &&
      (data.find("\ndata: ") == string::npos)) {
    return true;
  }

  // Validate id filters if we have any.
  if (_isIdFiltered) {
    string eventId = GetSSEField(data, "id");
    if (eventId.empty() || !isSubscribed(eventId, SUBSCRIPTION_ID)) return false;
  }

  // Validate event filters if we have any.
  if (_isEventFiltered) {
    string eventType = GetSSEField(data, "event");
    if (eventType.empty() || !isSubscribed(eventType, SUBSCRIPTION_EVENT_TYPE)) return false;
  }

  return true;
}
