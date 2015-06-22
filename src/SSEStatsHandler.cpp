#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <unistd.h>
#include <string>
#include <sstream>
#include "Common.h"
#include "SSEChannel.h"
#include "SSEServer.h"
#include "SSEClient.h"
#include "SSEStatsHandler.h"
#include "HTTPResponse.h"

/**
  Constructor.
*/
SSEStatsHandler::SSEStatsHandler() {
  this->config = NULL;
  this->server = NULL;

}

/**
 Destructor.
*/
SSEStatsHandler::~SSEStatsHandler() {
}

/**
  Initializes the handler, needs to be called before Run().
  @param config Pointer to SSEConfig instance.
  @param server Pointer to SSEServer instance.
*/
void SSEStatsHandler::Init(SSEConfig* config, SSEServer* server) {
  this->config = config;
  this->server = server;
}

/*
 Generate and return the statistics as JSON.
*/
const std::string SSEStatsHandler::GetJSON() {
  boost::property_tree::ptree pt; 
  boost::property_tree::ptree channels; 
  SSEChannelList::const_iterator it;

  long totalClients = 0;
  long totalEvents  = 0;
  int  numChannels  = 0;

  for (it = server->ChannelsBegin(); it != server->ChannelsEnd(); it++) {
    boost::property_tree::ptree pt_element; 
    SSEChannel* chan = (*it).get();
    SSEChannelStats stat;

    chan->GetStats(&stat);

    totalClients += stat.num_clients;
    totalEvents  += stat.num_broadcasted_events;

    pt_element.put("id", chan->GetId());
    pt_element.put("clients", stat.num_clients);
    pt_element.put("broadcasted_events", stat.num_broadcasted_events);
    pt_element.put("cached_events", stat.num_cached_events);
    pt_element.put("cache_size", stat.cache_size);
    
    channels.push_back(std::make_pair("", pt_element));
    numChannels++;
  }

  pt.put("global.clients", totalClients);
  pt.put("global.broadcasted_events", totalEvents);
  pt.put("global.channels", numChannels);

  if (numChannels > 0) {
    pt.add_child("channels", channels);
  }

  std::stringstream ss;
  write_json(ss, pt);

  return ss.str();
}

/**
 Send server statistics as JSON to client.
 @param client Pointer to SSEClient.
*/
void SSEStatsHandler::SendToClient(SSEClient* client) {
  HTTPResponse res;

  res.SetHeader("Content-Type", "application/json");
  res.SetHeader("Cache-Control", "no-cache");
  res.SetBody(GetJSON());

  client->Send(res.Get());
} 
