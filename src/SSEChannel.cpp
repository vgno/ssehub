#include "Common.h"
#include "SSEChannel.h"
#include "SSEClient.h"
#include "SSEClientHandler.h"
#include "SSEEvent.h"
#include "SSEConfig.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

using namespace std;
extern int stop;

/**
  Constructor.
  @param conf Pointer to SSEConfig instance holding our configuration.
  @param id Unique identifier for this channel.
*/
SSEChannel::SSEChannel(ChannelConfig conf, string id) {
  _config = conf;
  _config.id = id;

  _efd = epoll_create1(0);
  LOG_IF(FATAL, _efd == -1) << "epoll_create1 failed.";

  // Initialize counters.
  _stats.num_clients            = 0;
  _stats.num_connects           = 0;
  _stats.num_disconnects        = 0;
  _stats.num_errors             = 0;
  _stats.num_cached_events      = 0;
  _stats.num_broadcasted_events = 0;
  _stats.cache_size             = _config.cacheLength;


  LOG(INFO) << "Initializing channel " << _config.id;
  LOG(INFO) << "Cache Adapter: " << _config.cacheAdapter;
  LOG(INFO) << "Cache length: " << _config.cacheLength;
  LOG(INFO) << "Threads per channel: " << _config.server->GetValue("server.threadsPerChannel");

  _allow_all_origins = (_config.allowedOrigins.size() < 1) ? true : false;

  BOOST_FOREACH(const std::string& origin, _config.allowedOrigins) {
    DLOG(INFO) << "Allowed origin: " << origin;
  }

  // Internet Explorer has a problem receiving data when using XDomainRequest before it has received 2KB of data.
  // The polyfills that account for this send a query parameter, evs_preamble to inform the server about this so it can respond with some initial data.
  // Initialize a buffer containing our response for this case.
  _evs_preamble_data[0] = ':';
  for (int i = 1; i <= 2048; i++) { _evs_preamble_data[i] = '.'; }
  _evs_preamble_data[2049] = '\n';
  _evs_preamble_data[2050] = '\n';
  _evs_preamble_data[2051] = '\0';

  InitializeCache();
  InitializeThreads();
}

/**
  Destructor.
*/
SSEChannel::~SSEChannel() {
  DLOG(INFO) << "SSEChannel destructor called.";
  CleanupThreads();
}

void SSEChannel::InitializeCache() {
  const string adapter = _config.cacheAdapter;
  if (adapter == "redis") {
    _cache_adapter = new Redis(_config.id, _config);
  } else if (adapter == "memory") {
    _cache_adapter = new Memory(_config);
  } else if (adapter == "leveldb") {
    _cache_adapter = new LevelDB(_config);
  }
}

/**
  Initialize client handler threads and also the thread that
  pings all connected clients on _config.red interval.
*/
void SSEChannel::InitializeThreads() {
  int i;

  _cleanupthread = boost::thread(boost::bind(&SSEChannel::CleanupMain, this));

  for (i = 0; i < _config.server->GetValueInt("server.threadsPerChannel"); i++) {
    _clientpool.push_back(ClientHandlerPtr(new SSEClientHandler(i)));
  }

  curthread = _clientpool.begin();
  _pingthread = boost::thread(boost::bind(&SSEChannel::Ping, this));
}

/**
  Stop and free up all running threads.
  Called by destructor.
*/
void SSEChannel::CleanupThreads() {
  pthread_cancel(_pingthread.native_handle());
  pthread_cancel(_cleanupthread.native_handle());
}

/**
  Return the id of this channel.
*/
string SSEChannel::GetId() {
  return _config.id;
}

/**
 Set correct CORS headers.
 @param req Request object.
 @param res Response object.
*/
void SSEChannel::SetCorsHeaders(HTTPRequest* req, HTTPResponse& res) {
  if (_allow_all_origins) {
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
  BOOST_FOREACH(const std::string& origin, _config.allowedOrigins) {
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
  struct epoll_event ev;
  int ret;

  DLOG(INFO) << "Adding client to channel " << GetId();

  // Send CORS headers.
  SetCorsHeaders(req, res);

   // Reply with CORS headers when we get a OPTIONS request.
  if (req->GetMethod().compare("OPTIONS") == 0) {
    client->Send(res.Get());
    client->Destroy();
    return;
  }

  // Disallow every other method than GET.
  if (req->GetMethod().compare("GET") != 0) {
    DLOG(INFO) << "Method: " << req->GetMethod();
    res.SetStatus(405, "Method Not Allowed");
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
    res.AppendBody(_evs_preamble_data);
  }

  client->Send(res.Get());

  // Send event history if requested.
  if (!lastEventId.empty()) {
    SendEventsSince(client, lastEventId);
  } else if (!req->GetQueryString("getcache").empty()) {
    SendCache(client);
  }
  
  client->DeleteHttpReq();

  ev.events   = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  ev.data.ptr = client;
  ret = epoll_ctl(_efd, EPOLL_CTL_ADD, client->Getfd(), &ev);

  if (ret == -1) {
    DLOG(ERROR) << "Failed to add client " << client->GetIP() << " to epoll event list.";
    client->Destroy();
    return;
  }
  
  INC_LONG(_stats.num_connects);

  // Add client to handler thread in a round-robin fashion.
  client->SetNoDelay();
  (*curthread)->AddClient(client);
  curthread++;

  if (curthread == _clientpool.end()) curthread = _clientpool.begin();
}

/**
  Broadcasts string to all connected clients.
  @param data String to broadcast.
*/
void SSEChannel::Broadcast(const string& data) {
  ClientHandlerList::iterator it;

  for (it = _clientpool.begin(); it != _clientpool.end(); it++) {
    (*it)->Broadcast(data);
  }
}

/**
  Broadcasts SSEvent to all connected clients.
  @param event Event to broadcast.
*/
void SSEChannel::BroadcastEvent(SSEEvent* event) {
  Broadcast(event->get());
  INC_LONG(_stats.num_broadcasted_events);

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
  if (_cache_adapter) {
    _cache_adapter->CacheEvent(event);
    _stats.num_cached_events = _cache_adapter->GetSizeOfCachedEvents();
  }
}

/**
  Send all events since a given event id to client.
  @param lastId Send all events since this id.
*/
void SSEChannel::SendEventsSince(SSEClient* client, string lastId) {
  deque<string>::const_iterator it;
  deque<string> events = _cache_adapter->GetEventsSinceId(lastId);

  client->Cork();

  BOOST_FOREACH(const string& event, events) {
    client->Send(event);
  }

  client->Uncork();
}

/**
  Send all cached events to client.
  @param client SSEClient.
*/
void SSEChannel::SendCache(SSEClient* client) {
 deque<string>::const_iterator it;
 deque<string> events = _cache_adapter->GetAllEvents();

  client->Cork();

  BOOST_FOREACH(const string& event, events) {
    client->Send(event);
  }

  client->Uncork();
}

/**
 Handle client disconnects and errors.
*/
void SSEChannel::CleanupMain() {
  boost::shared_ptr<struct epoll_event[]> t_events(new struct epoll_event[1024]);

  while(!stop) {
    int n = epoll_wait(_efd, t_events.get(), 1024, -1);

    for (int i = 0; i < n; i++) {
      SSEClient* client;
      client = static_cast<SSEClient*>(t_events[i].data.ptr);

      if (t_events[i].events & EPOLLIN) {
        char buf[512];
        int rcv_len = client->Read(buf, 512);
        if (rcv_len <= 0) {
          client->MarkAsDead();
          INC_LONG(_stats.num_disconnects);
        }
      } else if ((t_events[i].events & EPOLLHUP)  || (t_events[i].events & EPOLLRDHUP)) {
        DLOG(INFO) << "Channel " << _config.id << ": Client disconnected.";
        client->MarkAsDead();
        INC_LONG(_stats.num_disconnects);
      } else if (t_events[i].events & EPOLLERR) {
        // If an error occours on a client socket, just drop the connection.
        DLOG(INFO) << "Channel " << _config.id << ": Error on client socket: " << strerror(errno);
        client->MarkAsDead();
        INC_LONG(_stats.num_errors);
      }
    }
  }
}

/**
  Sends a ping to all clients connected to this channel.
*/
void SSEChannel::Ping() {
  while(!stop) {
    Broadcast(":\n\n");
    sleep(_config.server->GetValueInt("server.pingInterval"));
  }
}

/**
  Returns number of clients connected to this channel.
*/
ulong SSEChannel::GetNumClients() {
  ClientHandlerList::iterator it;
  ulong numclients = 0;

  for (it = _clientpool.begin(); it != _clientpool.end(); it++) {
    numclients += (*it)->GetNumClients();
  }

  return numclients;
}

/**
 Fetch various statistics for the channel.
 @param stats Pointer to SSEChannelStats struct which is to be filled with the statistics.
**/
const SSEChannelStats& SSEChannel::GetStats() {
  _stats.num_clients = GetNumClients();
  return _stats;
}

/**
  Returns the ChannelConfig.
**/
const ChannelConfig& SSEChannel::GetConfig() {
  return _config;
}
