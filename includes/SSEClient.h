#ifndef SSECLIENT_H
#define SSECLIENT_H

#include <string>
#include <vector>
#include <sys/epoll.h>
#include <netinet/in.h>

/*
#include <boost/uuid/uuid.hpp> 
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
*/

using namespace std;

class SSEClient {
  public:
    SSEClient(int, struct sockaddr_in* csin);
    ~SSEClient();
    int Send(const string &data);
    size_t Read(void* buf, int len);
    //boost::uuids::uuid& GetId();
    int Getfd();
    const string GetIP();
    void Destroy();

  private:
    //boost::uuids::uuid uuid;
    int fd;
    struct sockaddr_in _csin;
};

#endif
