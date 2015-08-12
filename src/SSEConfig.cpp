#include <sstream>
#include <stdlib.h>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/any.hpp>
#include <boost/optional.hpp>
#include "Common.h"
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
 ConfigMap["server.allowUndefinedChannels"]   = "true";

 ConfigMap["amqp.host"]                       = "127.0.0.1";
 ConfigMap["amqp.port"]                       = "5672";
 ConfigMap["amqp.user"]                       = "guest";
 ConfigMap["amqp.password"]                   = "guest";
 ConfigMap["amqp.exchange"]                   = "amq.fanout";

ConfigMap["redis.host"]                       = "127.0.0.1";
ConfigMap["redis.port"]                       = "6379";

 ConfigMap["default.cacheAdapter"]            = "redis";
 ConfigMap["default.cacheLength"]             = "500";
}

/**
  Load a config file.
  @param file Filename of config to load. 
*/
bool SSEConfig::load(const char *file) {
  boost::property_tree::ptree pt;

  // Read config file.
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

  // Load channels defined in configuration.
  LoadChannels(pt);

  return true;
}

/**
  Load static configured channels from config.
  @param pt Configuration ptree.
*/
void SSEConfig::LoadChannels(boost::property_tree::ptree& pt) {
  // Set up DefaultChannelConfig.
  DefaultChannelConfig.server = this;
  DefaultChannelConfig.cacheAdapter = GetValue("default.cacheAdapter");
  DefaultChannelConfig.cacheLength = GetValueInt("default.cacheLength");

  // Get default allowedOrigins.
  try {
    GetArray(DefaultAllowedOrigins, pt.get_child("default.allowedOrigins"));
  } catch(boost::property_tree::ptree_error& e) {
    LOG(FATAL) << "Failed to fetch default allowedOrigins from config: " << e.what();
  }

  // Populate ChannelMap.
  try {
   BOOST_FOREACH(boost::property_tree::ptree::value_type& child, pt.get_child("channels")) {
    string chName;

    // Get channel name.
    try {
      chName = child.second.get<std::string>("path");
    } catch (boost::property_tree::ptree_error& e) {
      LOG(FATAL) << "Invalid channel definition in config: " << e.what();
    }

    // Pointer to this SSEConfig instance.
    ChannelMap[chName].server = this;

    // Get allowedOrigins for the channel or use the default.
    try {
      GetArray(ChannelMap[chName].allowedOrigins, child.second.get_child("allowedOrigins"));
    } catch (...) {
      ChannelMap[chName].allowedOrigins = DefaultAllowedOrigins;
    }

    // Optional channel parameters.
    ChannelMap[chName].cacheAdapter = child.second.get<std::string>("cacheAdapter", DefaultChannelConfig.cacheAdapter);
    ChannelMap[chName].cacheLength = child.second.get<int>("cacheLength", DefaultChannelConfig.cacheLength);
   }
  } catch(...) {
    if (!GetValueBool("server.allowUndefinedChannels")) {
      DLOG(FATAL) << "Error: No channels defined in config and allowUndefinedChannels is disabled.";
    }
  }
}

void SSEConfig::GetArray(vector<std::string>& target, boost::property_tree::ptree& pt) {
  BOOST_FOREACH(boost::property_tree::ptree::value_type& child, pt) {
    target.push_back(child.second.get_value<std::string>());
  }
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

/**
  Returns ChannelMap.
*/
ChannelMap_t& SSEConfig::GetChannels() {
  return ChannelMap;
}

/**
 Returns config for dynamicly added channels.
*/
ChannelConfig& SSEConfig::GetDefaultChannelConfig() {
  return DefaultChannelConfig;
}
