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
size_t SSEClient::_prune_sendbuffer_bytes(size_t bytes) {
  size_t bytes_used = 0, total_bytes = 0;
  unsigned int i = 0;

  // Return if requested bytes to remove is less than 1.
  if (bytes < 1) return 0;

  // Count total number of string bytes in _sndBuf.
  for (vector<string>::iterator it = _sndBuf.begin();
      it != _sndBuf.end(); total_bytes += it->length(), it++);

  // If requested bytes to remove is equal to or more than actual bytes present in _sndBuf clear it.
  if (bytes >= total_bytes) {
    _sndBuf.clear();
    return 0;
  }

  // Loop over _sndBuf and remove requested bytes.
  for (vector<string>::iterator it = _sndBuf.begin(); it != _sndBuf.end(); it++, i++) {

    if ((bytes_used + it->length()) > bytes) {
      size_t used_here = bytes-bytes_used;
      size_t rem_bytes = it->length() - used_here;

      *it = it->substr(used_here, rem_bytes);

      if (i > 0) {
        _sndBuf.erase(_sndBuf.begin(), it);
      }

      break;
    }

    bytes_used += (*it).length();
  }

  // Return elements present in _sndBuf after removal.
  return _sndBuf.size();
}

/*
 Cut off @param items from our internal sendbuffer.
 @param items Number of items to cut off.
*/
size_t SSEClient::_prune_sendbuffer_items(size_t items) {
  vector<string>::iterator it;

  // If requested items to remove is more than actual present in _sndBuf clear it.
  if (items > _sndBuf.size()) {
    _sndBuf.clear();
    return 0;
  }

  it  = _sndBuf.begin();
  it += items;

  _sndBuf.erase(_sndBuf.begin(), it);

  return _sndBuf.size();
}

/*
  Try to write all elements in the sendbuffer using writev().
*/
int SSEClient::_writev_sndbuf() {
  int ret = 0;

  // If we have multiple items, use writev to write
  // IOVEC_SIZE chunks of _sndBuf.
  for (unsigned int i = 0; i <= (_sndBuf.size() / IOVEC_SIZE); i++) {
    struct iovec siov[IOVEC_SIZE];
    size_t siov_cnt = 0;
    size_t data_len = 0;

    BOOST_FOREACH(const string& buf, _sndBuf) {
      if (siov_cnt == IOVEC_SIZE) { break; }
      siov[siov_cnt].iov_base  = (char*)buf.c_str();
      siov[siov_cnt].iov_len   = buf.length();
      data_len                += buf.length();
      siov_cnt++;
    }

    ret = writev(_fd, siov, siov_cnt);

    if (ret <= 0) {
      DLOG(INFO) << GetIP() << ": writev flush error: " << strerror(errno);
      break;
    }

    if ((unsigned int)ret < data_len) {
      // Remove data that was written from _sndBuf.
      _prune_sendbuffer_bytes(ret);
      DLOG(INFO) << GetIP() << ": Could not writev() entire buffer, wrote " << ret << " of " << data_len << " bytes.";
      break;
    }

    // Remove sent elements from _sndBuf.
    _prune_sendbuffer_items(siov_cnt);
  }

  return ret;
}

/*
  Write single element in the send buffer using write().
*/
int SSEClient::_write_sndbuf() {
  int ret = 0;
  size_t data_len = 0;

  data_len = _sndBuf.front().length();
  ret = write(_fd, _sndBuf.front().c_str(), data_len);

  if (ret <= 0) {
    DLOG(INFO) << GetIP() << ": write flush error: " << strerror(errno);
  } else if ((unsigned int)ret < data_len) {
    _prune_sendbuffer_bytes(ret);
    DLOG(INFO) << GetIP() << ": Could not write() entire buffer, wrote " << ret << " of " << data_len << " bytes.";
  } else {
    _sndBuf.clear();
  }

  return ret;
}

/*
 Flush data in the sendbuffer.
*/
int SSEClient::Flush() {
  boost::mutex::scoped_lock lock(_sndBufLock);

  if (_sndBuf.size() < 1) {
    return 0;
  }

  if (_sndBuf.size() == 1) {
    return _write_sndbuf();
  }

  return _writev_sndbuf();
}

/**
 Sends data to client.
 @param data String buffer to send.
*/
int SSEClient::Send(const string &data, bool flush) {
  if (!isFilterAcceptable(data)) return 0;

  _sndBufLock.lock();
  _sndBuf.push_back(data);
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
  if (!IsDead()) close(_fd);
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

/*
  Get clients sockaddr.
*/
uint32_t SSEClient::GetSockAddr() {
  return _csin.sin_addr.s_addr;
}

/*
 Get the HTTPRequest the client was initiated with.
*/
HTTPRequest* SSEClient::GetHttpReq() {
  return m_httpReq.get();
}

/*
  Delete the stored initial HTTPRequest.
*/
void SSEClient::DeleteHttpReq() {
  m_httpReq.reset();
}

/*
 Mark client as dead and ready for removal.
*/
void SSEClient::MarkAsDead() {
  _dead = true;
  close(_fd);
}

/*
 Returns wether this client is dead or not.
*/
bool SSEClient::IsDead() {
  return _dead;
}

/*
 Extract SSE-field from data.
 @param data Search string.
 @param fieldName field to extract.
*/
const string SSEClient::_get_sse_field(const string& data, const string& fieldName) {
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

/*
 Check if client is subscribed to a certain event.
 @param key Subscription key.
 @param type Subscription type.
*/
bool SSEClient::isSubscribed(const string key, SubscriptionType type) {
 BOOST_FOREACH(const SubscriptionElement& subscription, _subscriptions) {
    if (subscription.type == type && (subscription.key.compare(0, subscription.key.length(), key) == 0)) {
      return true;
    }
  }

 return false;
}

/*
  Subscripe to a certain event and type.
  @param key Subscription key.
  @param type Subscription type.
*/
void SSEClient::Subscribe(const string key, SubscriptionType type) {
  SubscriptionElement subscription;
  subscription.key  = key;
  subscription.type = type;
  if (type == SUBSCRIPTION_ID) _isIdFiltered = true;
  if (type == SUBSCRIPTION_EVENT_TYPE) _isEventFiltered = true;
  _subscriptions.push_back(subscription);
}

/*
  Check if data is allowed to pass our subscriptions.
  @param data Buffer to validate.
*/
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
    string eventId = _get_sse_field(data, "id");
    if (eventId.empty() || !isSubscribed(eventId, SUBSCRIPTION_ID)) return false;
  }

  // Validate event filters if we have any.
  if (_isEventFiltered) {
    string eventType = _get_sse_field(data, "event");
    if (eventType.empty() || !isSubscribed(eventType, SUBSCRIPTION_EVENT_TYPE)) return false;
  }

  return true;
}
