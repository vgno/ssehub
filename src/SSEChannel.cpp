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
void SSEChannel::BroadcastEvent(SSEvent event) {
 DLOG(INFO) << "Channel " << id << ": " << "[" << event.id << "] " << event.data;
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
