#ifndef CONFIG_H
#define CONFIG_H
#include <vector>
#include <string>

using namespace std;

struct SSEServerConfig {
  int port;
  string bindIP;
  string logDir;
  int    pingInterval;
  int    threadsPerChannel;
};

struct SSEAmqpConfig {
  string  host;
  string  exchange;
  string  user;
  string  password;
  int     port;
};

class SSEConfig {
  public:
    bool load(const char*);
    SSEConfig(string);
    SSEServerConfig& getServer();
    SSEAmqpConfig& getAmqp();

  private:
    SSEServerConfig serverConfig;
    SSEAmqpConfig   amqpConfig;
};

#endif
