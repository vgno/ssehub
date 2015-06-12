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
    SSEStatsHandler();
    ~SSEStatsHandler();
    void Init(SSEConfig* config, SSEServer* server);
    const std::string GetJSON();
    void SendToClient(SSEClient* client);

  private:
    SSEConfig* config;
    SSEServer* server;
};

#endif
