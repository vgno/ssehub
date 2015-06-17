#include <glog/logging.h>
#include <sstream>
#include <stdlib.h>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/any.hpp>
#include "SSEConfig.h"

using namespace std;

/**
  Constructor.
*/
SSEConfig::SSEConfig() {
  InitDefaults();
}

/**
  Initialize some sane default config values.
*/
void SSEConfig::InitDefaults() {

 ConfigMap["server.bindip"]                   = "0.0.0.0";
 ConfigMap["server.port"]                     = "8090";
 ConfigMap["server.logdir"]                   = "./";
 ConfigMap["server.pingInterval"]             = "5";
 ConfigMap["server.threadsPerChannel"]        = "5";
 ConfigMap["server.channelCacheSize"]         = "500";
 ConfigMap["server.allowUndefinedChannels"]   = "true";

 ConfigMap["amqp.host"]                       = "127.0.0.1";
 ConfigMap["amqp.port"]                       = "5672";
 ConfigMap["amqp.user"]                       = "guest";
 ConfigMap["amqp.password"]                   = "guest";
 ConfigMap["amqp.exchange"]                   = "amq.fanout";

}

/**
  Load a config file.
  @param file Filename of config to load. 
*/
bool SSEConfig::load(const char *file) {
  boost::property_tree::ptree pt;

  try {
    boost::property_tree::read_json(file, pt);
  } catch (...) {
    return false;
  }

  // Populate ConfigMap.
  BOOST_FOREACH(ConfigMap_t::value_type &element, ConfigMap) {
    try {
      string val = pt.get<std::string>(element.first);
      ConfigMap[element.first] = val;
      DLOG(INFO) << element.first << " = " << ConfigMap[element.first];
    } catch (...) {}
  }

  // Populate ChannelMap.
  try {
   BOOST_FOREACH(boost::property_tree::ptree::value_type& child, pt.get_child("channels")) {
    try {
      string chName = child.second.get<std::string>("path");
      string allowedOrigins = child.second.get<std::string>("allowedOrigins");
      int historyLength = child.second.get<int>("historyLength");

      ChannelMap[chName].allowedOrigins = allowedOrigins;
      ChannelMap[chName].historyLength = historyLength;
      } catch (boost::property_tree::ptree_error &e) {
        LOG(FATAL) << "Invalid channel definition in config: " << e.what();
      }
    }
  } catch(...) {
    DLOG(INFO) << "Warning: No channels defined in config.";
  }

  return true;
}

/**
  Fetch a config attribute and return as a string.
  @param key Config attribute to fetch. 
*/
const string &SSEConfig::GetValue(const string& key) {
  return ConfigMap[key];
}

/**
  Fetch a config attribute and return as a int.
  @param key Config attribute to fetch. 
*/
int SSEConfig::GetValueInt(const string& key) {
  try  {
    return boost::lexical_cast<int>(ConfigMap[key]);
  } catch(...) {
    return 0;
  }
}

/**
 *  Fetch a config attribute and return as a boolean.
 *  @param key Config attribute to fetch.
*/
bool SSEConfig::GetValueBool(const string& key) {
    if (ConfigMap[key].compare("true") == 0) return true;
      return false;
}
