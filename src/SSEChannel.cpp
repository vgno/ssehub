#include "Common.h"
#include "SSEChannel.h"
#include "SSEClient.h"
#include "SSEClientHandler.h"
#include "SSEEvent.h"
#include "SSEConfig.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include <boost/foreach.hpp>

using namespace std;

/**
  Constructor.
  @param conf Pointer to SSEConfig instance holding our configuration.
  @param id Unique identifier for this channel.
*/
SSEChannel::SSEChannel(ChannelConfig& conf, string id) {
  this->id = id;
  this->config = conf;
  num_broadcasted_events = 0;
  LOG(INFO) << "Initializing channel " << id;
  LOG(INFO) << "History length: " << config.historyLength;
  LOG(INFO) << "History URL: " << config.historyUrl;
  LOG(INFO) << "Threads per channel: " << config.server->GetValue("server.threadsPerChannel");

  allowAllOrigins = (config.allowedOrigins.size() < 1) ? true : false;

  BOOST_FOREACH(const std::string& origin, config.allowedOrigins) {
    DLOG(INFO) << "Allowed origin: " << origin;     
  }

  // Internet Explorer has a problem receiving data when using XDomainRequest before it has received 2KB of data. 
  // The polyfills that account for this send a query parameter, evs_preamble to inform the server about this so it can respond with some initial data.
  // Initialize a buffer containing our response for this case.
  evs_preamble_data[0] = ':';
  for (int i = 1; i <= 2048; i++) { evs_preamble_data[i] = '.'; }
  evs_preamble_data[2049] = '\n';
  evs_preamble_data[2050] = '\n';
  evs_preamble_data[2051] = '\0';

  InitializeThreads();
}

/**
  Destructor.
*/
SSEChannel::~SSEChannel() {
  DLOG(INFO) << "SSEChannel destructor called.";
  CleanupThreads();
}

/**
  Initialize client handler threads and also the thread that
  pings all connected clients on configured interval.
*/
void SSEChannel::InitializeThreads() {
  int i;

  for (i = 0; i < config.server->GetValueInt("server.threadsPerChannel"); i++) {
    clientpool.push_back(ClientHandlerPtr(new SSEClientHandler(i)));
  }

  curthread = clientpool.begin();
  pthread_create(&_pingthread, NULL, &SSEChannel::PingThread, this); 
}

/**
  Stop and free up all running threads.
  Called by destructor.
*/
void SSEChannel::CleanupThreads() {
  pthread_cancel(_pingthread);
}

/**
  Return the id of this channel.
*/
string SSEChannel::GetId() {
  return id;
}

/**
 Set correct CORS headers.
 @param req Request object.
 @param res Response object.
*/
void SSEChannel::SetCorsHeaders(HTTPRequest* req, HTTPResponse& res) {
  if (allowAllOrigins) {
    DLOG(INFO) << "All origins allowed.";
    res.SetHeader("Access-Control-Allow-Origin", "*");
    return;
  }

  const string& originHeader = req->GetHeader("Origin");

  // If Origin header is not set in the request don't set any CORS headers.
  if (originHeader.empty()) {
    DLOG(INFO) << "No Origin header in request, not setting cors headers.";
    return;
  }

  // If Origin matches one of the origins in the allowedOrigins array use that in the CORS header.
  BOOST_FOREACH(const std::string& origin, config.allowedOrigins) {
    if (originHeader.compare(0, origin.size(), origin) == 0) {
      res.SetHeader("Access-Control-Allow-Origin", origin);
      DLOG(INFO) << "Referer matches origin " << origin;
      return;
    }
  }
}

/**
  Adds a client to one of the client handlers assigned to the channel.
  Clients is distributed evenly across the client handler threads.
  @param client SSEClient pointer.
*/
void SSEChannel::AddClient(SSEClient* client, HTTPRequest* req) {
  HTTPResponse res;
  DLOG(INFO) << "Adding client to channel " << GetId();

  // Send CORS headers.
  SetCorsHeaders(req, res);

  // Disallow every other method than GET.
  if (req->GetMethod().compare("GET") != 0) {
    DLOG(INFO) << "Method: " << req->GetMethod();
    res.SetStatus(405, "Method Not Allowed");
    client->Send(res.Get());
    client->Destroy();
    return;
  }

  // Reply with CORS headers when we get a OPTIONS request.
  if (req->GetMethod().compare("OPTIONS") == 0) {
    res.SetHeader("Access-Control-Allow-Origin", "*");
    client->Send(res.Get());
    client->Destroy();
    return;
  }

  string lastEventId = req->GetHeader("Last-Event-ID");
  if (lastEventId.empty()) lastEventId = req->GetQueryString("evs_last_event_id");
  if (lastEventId.empty()) lastEventId = req->GetQueryString("lastEventId");

  // Send initial response headers, etc.
  res.SetHeader("Content-Type", "text/event-stream");
  res.SetHeader("Cache-Control", "no-cache");
  res.SetHeader("Connection", "keep-alive");
  res.SetBody(":ok\n\n");

  // Send preamble if polyfill require it.
  if (!req->GetQueryString("evs_preamble").empty()) {
    res.AppendBody(evs_preamble_data);
  }

  client->Send(res.Get());
  client->DeleteHttpReq();

  // Send event history if requested.
  if (!lastEventId.empty()) {
    SendEventsSince(client, lastEventId);
  }

  // Add client to handler thread in a round-robin fashion.
  (*curthread)->AddClient(client); 
  curthread++;

  if (curthread == clientpool.end()) curthread = clientpool.begin();
}

/**
  Broadcasts string to all connected clients.
  @param data String to broadcast.
*/
void SSEChannel::Broadcast(const string& data) {
  ClientHandlerList::iterator it;

  for (it = clientpool.begin(); it != clientpool.end(); it++) {
    (*it)->Broadcast(data);
  }
}

/**
  Broadcasts SSEvent to all connected clients.
  @param event Event to broadcast.
*/
void SSEChannel::BroadcastEvent(SSEEvent* event) {
  Broadcast(event->get());
  num_broadcasted_events++;

  //Add event to cache if it contains a id field.
  if (!event->getid().empty()) {
    CacheEvent(event);
  } else {
    delete(event);
  } 
}

/**
  Add event to cache.
  @param event Event to cache.
*/
void SSEChannel::CacheEvent(SSEEvent* event) {
  // If we have the event id in our vector already don't remove it.
  // We want to keep the order even if we get an update on the event.
  if (std::find(cache_keys.begin(), cache_keys.end(), event->getid()) == cache_keys.end()) {
    cache_keys.push_back(event->getid());
  }

  cache_data[event->getid()] = SSEEventPtr(event);

  // Delete the oldest cache object if we hit the historyLength limit.
  if ((int)cache_keys.size() > config.historyLength) {
    string& firstElementId = *(cache_keys.begin());
    cache_data.erase(firstElementId);
    cache_keys.erase(cache_keys.begin());
  }
}

/**
  Send all events since a given event id to client.
  @param lastId Send all events since this id.
*/
void SSEChannel::SendEventsSince(SSEClient* client, string lastId) {
  deque<string>::const_iterator it;

  it = std::find(cache_keys.begin(), cache_keys.end(), lastId);
  if (it == cache_keys.end()) return;

  while (it != cache_keys.end()) {
    client->Send(cache_data[*(it)]->get());
    it++;
  }
}

/**
  Wrapper function for Ping.
*/
void *SSEChannel::PingThread(void *pThis) {
  SSEChannel *pt = static_cast<SSEChannel*>(pThis);
  pt->Ping();
  return NULL;
}

/**
  Sends a ping to all clients connected to this channel.
*/
void SSEChannel::Ping() {
  while(1) {
    Broadcast(":\n\n");
    sleep(config.server->GetValueInt("server.pingInterval"));
  }
}

/**
 Fetch various statistics for the channel.
 @param stats Pointer to SSEChannelStats struct which is to be filled with the statistics.
**/
void SSEChannel::GetStats(SSEChannelStats* stats) {
  // Reset counters.
  stats->num_clients      = 0;
  stats->num_connects     = 0;
  stats->num_disconnects  = 0;
  stats->num_errors       = 0;

  // Cache and broadcast counters.
  stats->num_broadcasted_events = num_broadcasted_events;
  stats->num_cached_events      = cache_keys.size();
  stats->cache_size             = config.historyLength;


  // Fetch statistics from client handlers.
  BOOST_FOREACH (const ClientHandlerPtr& handler, clientpool) {
    const SSEClientHandlerStats& handlerStats = handler->GetStats();

    stats->num_clients     += handlerStats.num_clients;
    stats->num_connects    += handlerStats.num_connects;
    stats->num_disconnects += handlerStats.num_disconnects;
    stats->num_errors      += handlerStats.num_errors;
  }
}
