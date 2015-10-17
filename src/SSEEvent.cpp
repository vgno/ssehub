#include <exception>
#include <boost/algorithm/string.hpp>
#include "Common.h"
#include "SSEEvent.h"

using namespace std;

SSEEvent::SSEEvent(const string& jsondata) {
  _json_ss << jsondata;
  _retry = 0;
}

SSEEvent::~SSEEvent() {

}

bool SSEEvent::compile() {
  boost::property_tree::ptree pt;
  string indata;

  try {
    boost::property_tree::read_json(_json_ss, pt);

    if (_path.empty()) { 
      _path = pt.get<std::string>("path"); 
    }

    indata = pt.get<std::string>("data"); // required.
  } catch (...) {
    return false;
  }

  try {
    _id = pt.get<std::string>("id");
    _event = pt.get<std::string>("event");
    _retry =  pt.get<int>("retry");
  } catch (...) {
    // Items not required, so don't fail.
  }

  boost::split(_data, indata, boost::is_any_of("\n"));

 return true;
}

const string SSEEvent::get() {
  stringstream ss;

  if (_data.empty() || _path.empty()) return "";

  if (!_id.empty()) ss << "id: " << _id << endl;
  if (!_event.empty()) ss << "event: " << _event << endl;
  if (_retry > 0) ss << "retry: " << _retry << endl;

  vector<string>::iterator it;
  for (it = _data.begin(); it != _data.end(); it++) {
    ss << "data: " << *it << endl;
  }

  ss << "\n";

  return ss.str();
}

void SSEEvent::setpath(const string path) {
  _path = path;
}

const string SSEEvent::getpath() {
  return _path;
}

const string SSEEvent::getid() {
  return _id;
}
