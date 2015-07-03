#ifndef SSESTATSHANDLER_H
#define SSESTATSHANDLER_H

#include <string>
#include "Common.h"

extern int stop;

// Forward declarations.
class SSEConfig;
class SSEServer;
class SSEClient;

class SSEStatsHandler {
  public:
    ulong invalid_events_rcv;
    ulong router_read_errors;
    ulong invalid_http_req;
    ulong oversized_http_req;

    SSEStatsHandler();
    ~SSEStatsHandler();
    void Init(SSEConfig* config, SSEServer* server);
    const std::string& GetJSON();
    void SendToClient(SSEClient* client);

  private:
    std::string _jsonData;
    SSEConfig* _config;
    SSEServer* _server;

    void Update();
};

#endif
