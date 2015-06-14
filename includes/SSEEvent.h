#ifndef SSEEVENT_H
#define SSEEVENT_H

#include <string>
#include <sstream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <glog/logging.h>

using namespace std;

class SSEEvent {
  public:
    SSEEvent(const string& jsonData);
    ~SSEEvent();
    bool  compile();
    const string get();
    const string getpath();
    const string getid();
  
  private:
    stringstream json_ss;
    string event;
    string path;
    vector<string> data;
    string id;
    int retry;
};

#endif
