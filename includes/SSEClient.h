#ifndef SSECLIENT_H
#define SSECLIENT_H

#include <string>
#include <vector>
#include <boost/uuid/uuid.hpp> 
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sys/epoll.h>

using namespace std;

class SSEClient {
  public:
    SSEClient(int);
    ~SSEClient();
    int Send(const string &data);
    const string Recv(int len);
    boost::uuids::uuid GetId();
    int Getfd();

  private:
    boost::uuids::uuid uuid;
    int fd;
};

typedef vector<SSEClient*> SSEClientPtrList;

#endif
