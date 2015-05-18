#ifndef CLIENT_H
#define CLIENT_H

#include <string>

using namespace std;

class SSEClient {
  private:
    int socket;

  public:
    SSEClient(int sock);
    ~SSEClient();
    int write(string data);
};

#endif
