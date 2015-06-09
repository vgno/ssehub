#include "SSEEvent.h"
#include <exception>
#include <boost/algorithm/string.hpp>

using namespace std;

SSEEvent::SSEEvent(const string& jsondata) {
  json_ss << jsondata;
  retry = 0;
}

SSEEvent::~SSEEvent() {
}

bool SSEEvent::compile() {
  boost::property_tree::ptree pt;
  string indata;

  try {
    boost::property_tree::read_json(json_ss, pt);
    indata = pt.get<std::string>("data"); // required.
  } catch (...) {
    return false;
  }  

  try {
    path = pt.get<std::string>("path"); 
    id = pt.get<std::string>("id");
    event = pt.get<std::string>("event");
    retry =  pt.get<int>("retry");
  } catch (...) {
    // Items not required, so don't fail.
  }


 boost::split(data, indata, boost::is_any_of("\n"));

 return true;
}

const string SSEEvent::get() {
  stringstream ss;

  if (data.empty() || path.empty()) return "";

 
  if (!id.empty()) ss << "id: " << id << endl;   
  if (!event.empty()) ss << "event: " << event << endl;   
  if (retry > 0) ss << "retry: " << retry << endl;

  vector<string>::iterator it;
  for (it = data.begin(); it != data.end(); it++) {
    ss << "data: " << *it << endl;
  }

  ss << "\n";

  return ss.str(); 
}

const string SSEEvent::getpath() {
  return path;
}

const string SSEEvent::getid() {
  return id;
}

