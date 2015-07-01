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
  @param conf Pointer to SSEConfig instance holding our _config.ration.
  @param id Unique identifier for this channel.
*/
SSEChannel::SSEChannel(ChannelConfig& conf, string id) {
  _id = id;
  _config = conf;

  _efd = epoll_create1(0);
  LOG_IF(FATAL, _efd == -1) << "epoll_create1 failed.";

  // Initialize counters.
  _stats.num_clients           = 0;
  _stats.num_connects          = 0;
  _stats.num_disconnects       = 0;
  _stats.num_errors            = 0;
  _stats.num_cached_events     = 0;
  _stats.num_broadcasted_events = 0;
  _stats.cache_size            = _config.historyLength;


  LOG(INFO) << "Initializing channel " << _id;
  LOG(INFO) << "History length: " << _config.historyLength;
  LOG(INFO) << "History URL: " << _config.historyUrl;
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
  return _id;
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
    res.AppendBody(_evs_preamble_data);
  }

  client->Send(res.Get());
  client->DeleteHttpReq();

  // Send event history if requested.
  if (!lastEventId.empty()) {
    SendEventsSince(client, lastEventId);
  }


  ev.events   = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLET;
  ev.data.ptr = client;
  ret = epoll_ctl(_efd, EPOLL_CTL_ADD, client->Getfd(), &ev);

  if (ret == -1) {
    DLOG(ERROR) << "Failed to add client " << client->GetIP() << " to epoll event list.";
    client->Destroy();
    return;
  }

  _clientlist.push_back(SSEClientPtr(client));
 
  _stats.num_clients++;
  INC_LONG(_stats.num_connects);

  // Add client to handler thread in a round-robin fashion.
  (*curthread)->AddClient(_clientlist.back());
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
  // If we have the event id in our vector already don't remove it.
  // We want to keep the order even if we get an update on the event.
  if (std::find(_cache_keys.begin(), _cache_keys.end(), event->getid()) == _cache_keys.end()) {
    _cache_keys.push_back(event->getid());
  }

  _cache_data[event->getid()] = SSEEventPtr(event);

  // Delete the oldest cache object if we hit the historyLength limit.
  if ((int)_cache_keys.size() > _config.historyLength) {
    string& firstElementId = *(_cache_keys.begin());
    _cache_data.erase(firstElementId);
    _cache_keys.erase(_cache_keys.begin());
  }

  _stats.num_cached_events = _cache_keys.size();
}

/**
  Send all events since a given event id to client.
  @param lastId Send all events since this id.
*/
void SSEChannel::SendEventsSince(SSEClient* client, string lastId) {
  deque<string>::const_iterator it;

  it = std::find(_cache_keys.begin(), _cache_keys.end(), lastId);
  if (it == _cache_keys.end()) return;

  while (it != _cache_keys.end()) {
    client->Send(_cache_data[*(it)]->get());
    it++;
  }
}

/**
  Remove a client from the clientlist.
  @param client Client to remove.
*/
void SSEChannel::RemoveClient(SSEClient* client) {
  SSEClientPtrList::iterator it;

  for (it = _clientlist.begin(); it != _clientlist.end(); it++) {
    if ((*it).get() == client) {
      _clientlist.erase(it);
      return;
    }
  }

  DLOG(ERROR) << "RemoveClient: " << "Failed.";
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

      if ((t_events[i].events & EPOLLHUP) || (t_events[i].events & EPOLLRDHUP)) {
        DLOG(INFO) << "Channel " << _id << ": Client disconnected.";
        RemoveClient(client);
        INC_LONG(_stats.num_disconnects);
        _stats.num_clients--;
      } else if (t_events[i].events & EPOLLERR) {
        // If an error occours on a client socket, just drop the connection.
        DLOG(INFO) << "Channel " << _id << ": Error on client socket: " << strerror(errno);
        RemoveClient(client);
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
 Fetch various statistics for the channel.
 @param stats Pointer to SSEChannelStats struct which is to be filled with the statistics.
**/
const SSEChannelStats& SSEChannel::GetStats() {
  return _stats;
}
