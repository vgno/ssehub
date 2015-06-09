#include "SSEChannel.h"
#include <iostream>

using namespace std;

/**
  Constructor.
  @param conf Pointer to SSEConfig instance holding our configuration.
  @param id Unique identifier for this channel.
*/
SSEChannel::SSEChannel(SSEConfig* conf, string id) {
  this->id = id;
  this->config = conf;
  DLOG(INFO) << "Initializing channel " << id;
  DLOG(INFO) << "Threads per channel: " << config->GetValue("server.threadsPerChannel");

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

  for (i = 0; i < config->GetValueInt("server.threadsPerChannel"); i++) {
    clientpool.push_back(new SSEClientHandler(i));
  }

  curthread = clientpool.begin();

  pthread_create(&_pingthread, NULL, &SSEChannel::PingThread, this); 
}

/**
  Stop and free up all running threads.
  Called by destructor.
*/
void SSEChannel::CleanupThreads() {
  ClientHandlerList::iterator it;

  pthread_cancel(_pingthread);

  for (it = clientpool.begin(); it != clientpool.end(); it++) { 
   delete(*it);
  }
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
void SSEChannel::AddClient(SSEClient* client) {
  DLOG(INFO) << "Adding client to channel " << GetId();

  // Send initial response headers, etc.
  client->Send("HTTP/1.1 200 OK\r\n");
  client->Send(header_data);
  client->Send("\r\n\r\n");
  client->Send(":ok\n\n");

  (*curthread)->AddClient(client); 

  *curthread++;
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
  // Add event to cache if it contains a id field.
  if (!event->getid().empty()) {
    CacheEvent(event);
  }

  Broadcast(event->get());

  delete(event);
}

/**
  Add event to cache.
  @param event Event to cache.
*/
void SSEChannel::CacheEvent(SSEEvent* event) {
  if ((int)cache_keys.size() == config->GetValueInt("server.channelCacheSize")) {
    cache_data.erase(*(cache_keys.begin()));
    cache_keys.erase(cache_keys.begin());
  }

  // If we have the event id in our vector already don't remove it.
  // We want to keep the order even if we get an update on the event.
  if (std::find(cache_keys.begin(), cache_keys.end(), event->getid()) == cache_keys.end()) {
    cache_keys.push_back(event->getid());
  }

  cache_data[event->getid()] = event->get();
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
    sleep(config->GetValueInt("server.pingInterval"));
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
