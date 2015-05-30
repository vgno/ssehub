#include <string>
#include <sstream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;

class SSEEvent {
  public:
    SSEEvent(const string& jsonData);
    bool  compile();
    const string get();

  private:
    stringstream json_ss;
    string event;
    string path;
    vector<string> data;
    string id;
    int retry;
    
};
