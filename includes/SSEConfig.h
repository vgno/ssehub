#ifndef CONFIG_H
#define CONFIG_H
#include <map>
#include <string>

using namespace std;

struct ChannelConfig {
  std::string allowedOrigins;
  int historyLength;
};

typedef std::map<const std::string, std::string> ConfigMap_t;
typedef std::map<const std::string, struct ChannelConfig> ChannelMap_t;

class SSEConfig {
  public:
    bool load(const char*);
    SSEConfig();
    const string &GetValue(const string& key);
    int GetValueInt(const string& key);
    bool GetValueBool(const string& key);

  private:
    void InitDefaults();
    ConfigMap_t ConfigMap;
    ChannelMap_t ChannelMap;
};

#endif
