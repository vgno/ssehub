#ifndef CONFIG_H
#define CONFIG_H
#include <map>
#include <string>
#include <stdint.h>
#include <boost/property_tree/ptree.hpp>

using namespace std;

typedef struct {
  uint32_t range;
  uint32_t mask;
} iprange_t;

struct ChannelConfig {
  string                 id;
  class SSEConfig*       server;
  std::vector<string>    allowedOrigins;
  std::vector<iprange_t> allowedPublishers;
  string                 cacheAdapter;
  int                    cacheLength;
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
    ChannelMap_t& GetChannels();
    ChannelConfig& GetDefaultChannelConfig();

  private:
    void InitDefaults();
    void GetArray(vector<std::string>& target, boost::property_tree::ptree& pt);
    void LoadChannels(boost::property_tree::ptree& pt);
    void GetAllowedPublishers(ChannelConfig& conf, boost::property_tree::ptree& pt);
    ConfigMap_t ConfigMap;
    ChannelMap_t ChannelMap;
    ChannelConfig DefaultChannelConfig;
    vector<std::string> DefaultAllowedOrigins;
};

#endif
