#ifndef SSECLIENT_H
#define SSECLIENT_H

#include <string>
#include <vector>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <boost/shared_ptr.hpp>
#include "HTTPRequest.h"

using namespace std;

class SSEClient {
  public:
    SSEClient(int, struct sockaddr_in* csin);
    ~SSEClient();
    int Send(const string &data);
    size_t Read(void* buf, int len);
    int Getfd();
    HTTPRequest* GetHttpReq();
    const string GetIP();
    void MarkAsDead();
    bool IsDead();
    void Destroy();
    void DeleteHttpReq();

  private:
    int _fd;
    struct sockaddr_in _csin;
    bool _dead;
    boost::shared_ptr<HTTPRequest> m_httpReq;
};

#endif
