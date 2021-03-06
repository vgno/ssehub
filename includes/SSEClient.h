#ifndef SSECLIENT_H
#define SSECLIENT_H

#include <string>
#include <deque>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdint.h>
#include <mutex>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "HTTPRequest.h"

#define IOVEC_SIZE 512
#define SND_NO_FLUSH false

enum SubscriptionType {
  SUBSCRIPTION_ID,
  SUBSCRIPTION_EVENT_TYPE
};

typedef struct {
  string key;
  SubscriptionType type;
} SubscriptionElement;

using namespace std;

class SSEClient {
  public:
    SSEClient(int fd, struct sockaddr_in* csin);
    ~SSEClient();
    ssize_t Send(const string &data);
    size_t Read(char* buf, int len);
    int Getfd();
    HTTPRequest* GetHttpReq();
    const string GetIP();
    uint32_t GetSockAddr();
    void MarkAsDead();
    bool IsDead();
    void Destroy();
    void DeleteHttpReq();
    bool isSubscribed(const string key, SubscriptionType type);
    void Subscribe(const string key, SubscriptionType type);
    bool isFilterAcceptable(const string& data);
    ssize_t Flush();
    int AddToEpoll(int epoll_fd, uint32_t events);

   private:
    int _fd;
    int _epoll_fd;
    struct epoll_event _epoll_event;
    struct sockaddr_in _csin;
    bool _dead;
    bool _isEventFiltered;
    bool _isIdFiltered;
    vector<SubscriptionElement> _subscriptions;
    boost::shared_ptr<HTTPRequest> m_httpReq;
    string _write_buffer;
    std::mutex _write_lock;
    size_t _prune_write_buffer(size_t bytes);
    void _enable_epoll_out();
    void _disable_epoll_out();
    const string _get_sse_field(const string& data, const string& fieldName);
};

#endif
