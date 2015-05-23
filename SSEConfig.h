#ifndef CONFIG_H
#define CONFIG_H
#include <vector>
#include <string>

using namespace std;

struct SSEServerConfig {
  int port;
  string bindIP;
  string logDir;
};

struct SSEChannelConfig {
  string  ID;
  string  amqpHost;
  string  amqpQueue;
  string  amqpUser;
  string  amqpPassword;
  int     amqpPort;
  int     pingInterval;
};

typedef vector<SSEChannelConfig> SSEChannelConfigList;

class SSEConfig {
  public:
    bool load(const char*);
    SSEChannelConfigList getChannels();
    SSEConfig(string);
    SSEServerConfig& getServer();

  private:
    SSEChannelConfigList chanConfList;
    SSEServerConfig serverConfig;
};

#endif