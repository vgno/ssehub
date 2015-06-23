#ifndef SSESTATSHANDLER_H
#define SSESTATSHANDLER_H

#include <string>

extern int stop;

// Forward declarations.
class SSEConfig;
class SSEServer;
class SSEClient;

class SSEStatsHandler {
  public:
    long invalid_events_rcv;
    long router_read_errors;
    long invalid_http_req;
    long oversized_http_req;

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
