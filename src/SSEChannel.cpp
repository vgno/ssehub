#include "SSEChannel.h"
#include "SSEClient.h"
#include "SSEClientHandler.h"
#include "SSEEvent.h"
#include "SSEConfig.h"
#include "HTTPRequest.h"

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
  DLOG(INFO) << "Initializing channel " << id;
  DLOG(INFO) << "History length: " << config.historyLength;
  DLOG(INFO) << "History URL: " << config.historyUrl;
  DLOG(INFO) << "Threads per channel: " << config.server->GetValue("server.threadsPerChannel");

  // Internet Explorer has a problem receiving data when using XDomainRequest before it has received 2KB of data. 
  // The polyfills that account for this send a query parameter, evs_preamble to inform the server about this so it can respond with some initial data.
  // Initialize a buffer containing our response for this case.
  evs_preamble_data[0] = ':';
  for (int i = 1; i <= 2048; i++) { evs_preamble_data[i] = '.'; }
  evs_preamble_data[2049] = '\n';
  evs_preamble_data[2050] = '\0';

  AddResponseHeader("Content-Type", "text/event-stream");
  AddResponseHeader("Cache-Control", "no-cache");
  AddResponseHeader("Connection", "keep-alive");
  AddResponseHeader("Access-Control-Allow-Origin", "*"); // This is temporary.
  CommitHeaderData();

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
  Adds a client to one of the client handlers assigned to the channel.
  Clients is distributed evenly across the client handler threads.
  @param client SSEClient pointer.
*/
void SSEChannel::AddClient(SSEClient* client, HTTPRequest* req) {
  DLOG(INFO) << "Adding client to channel " << GetId();

  // Reply with CORS headers when we get a OPTIONS request.
  if (req->GetMethod().compare("OPTIONS") == 0) {
    client->Send("HTTP/1.1 200 OK\r\n");
    client->Send("Access-Control-Allow-Origin: *\r\n");
    client->Send("Connection: close\r\n\r\n");
    client->Destroy();
    return;
  }

  // Disallow every other method than GET.
  if (req->GetMethod().compare("GET") != 0) {
    client->Send("HTTP/1.1 405 Method Not Allowed\r\n");
    client->Send("Connection: close\r\n\r\n");
    client->Destroy();
    return;
  }

  string lastEventId = req->GetHeader("Last-Event-ID");
  if (lastEventId.empty()) lastEventId = req->GetQueryString("evs_last_event_id");
  if (lastEventId.empty()) lastEventId = req->GetQueryString("lastEventId");

  // Send initial response headers, etc.
  client->Send("HTTP/1.1 200 OK\r\n");
  client->Send(header_data);
  client->Send("\r\n");
  client->Send(":ok\n\n");

  // Send preamble if polyfill require it.
  if (!req->GetQueryString("evs_preamble").empty()) {
    client->Send(evs_preamble_data);
  }

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
    Broadcast(":\n");
    sleep(config.server->GetValueInt("server.pingInterval"));
  }
}

/**
  Add header to response.
  @param header Header name.
  @param val Header content.
*/
void SSEChannel::AddResponseHeader(const string& header, const string& val) {
  request_headers[header] = val;
}

/**
 Composes a string (this->header_data) of the headers added by AddResponseHeader.
*/
void SSEChannel::CommitHeaderData() {
  stringstream header_data_ss;
  map<string, string>::iterator it;

  for (it = request_headers.begin(); it != request_headers.end(); it++) {
    header_data_ss << it->first << ": " << it->second << "\r\n"; 
  }

  header_data = header_data_ss.str();
}

/**
 Fetch various statistics for the channel.
 @param stats Pointer to SSEChannelStats struct which is to be filled with the statistics.
**/
void SSEChannel::GetStats(SSEChannelStats* stats) {
  ClientHandlerList::iterator it;
  long num_clients = 0;

  for (it = clientpool.begin(); it != clientpool.end(); it++) {
    num_clients += (*it)->GetNumClients();
  }

  stats->num_clients            = num_clients;
  stats->num_broadcasted_events = num_broadcasted_events;
  stats->num_cached_events      = cache_keys.size();
  stats->cache_size             = config.historyLength;
}
