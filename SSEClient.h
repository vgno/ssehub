#ifndef SSECLIENT_H
#define SSECLIENT_H

#include <string>
#include <boost/uuid/uuid.hpp> 
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/shared_ptr.hpp>
#include <sys/epoll.h>

using namespace std;

class SSEClient {
  public:
    SSEClient(int);
    ~SSEClient();
    void Send(const string &data);
    const string Recv(int len);
    struct epoll_event *GetEvent();

  private:
    boost::uuids::uuid uuid;
    int fd;
    struct epoll_event ev;
};

typedef boost::shared_ptr<SSEClient> SSEClientPtr;

#endif
