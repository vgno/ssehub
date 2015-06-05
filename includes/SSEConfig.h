#ifndef CONFIG_H
#define CONFIG_H
#include <map>
#include <string>

using namespace std;

class SSEConfig {
  public:
    bool load(const char*);
    SSEConfig(string);
    const string &GetValue(const string& key);
    int GetValueInt(const string& key);

  private:
    void InitDefaults();
    map<string, string> ConfigKeys;
};

#endif
