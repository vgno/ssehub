#include "Common.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
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

  int flag = 1;

  // By default limit max chunk size to 16kB.
  _sndBufSize = 16384;

  _isEventFiltered = false;
  _isIdFiltered = false;

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

/**
 Sends data to client.
 @param data String buffer to send.
*/
int SSEClient::Send(const string &data) {
  boost::mutex::scoped_lock lock(_writelock);
  int ret;
  unsigned int dataWritten;;

  if (!isFilterAcceptable(data)) return 0;

  // Split data into _sndBufSize chunks.
  for(unsigned int i = 0; i < data.length(); i += _sndBufSize) {
      string chunk = data.substr(i, _sndBufSize);
      dataWritten = 0;

      // Run send() until the whole chunk is transmitted.
      do {
        ret = send(_fd, chunk.c_str()+dataWritten, chunk.length()-dataWritten, MSG_NOSIGNAL);     

        if (ret < 0) {
          if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { 
            continue;
          } else {
            DLOG(ERROR) << "Error sending data to client with IP " << GetIP() << ": " << strerror(errno);
            return ret;
          }
        }

        dataWritten += ret;
      } while (dataWritten < chunk.size());
  }

  return dataWritten;
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
  if ((data.compare(0, 6, "data: ") == 0) ||
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
