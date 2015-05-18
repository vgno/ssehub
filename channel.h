#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
#include <string>
#include "client.h"

using namespace std;

typedef struct {
  long timestamp;
  string body;
} SSEvent;

typedef struct {
  string ID;
  int pingInterval;
} ChannelConfig;

class SSEChannel {
  public:
    SSEChannel();
    string getID();
    void broadcast(SSEvent *event);

  private:
    string ID;
    vector<SSEClient> clientList;
    vector<SSEvent> eventHistory;
};

#endif
