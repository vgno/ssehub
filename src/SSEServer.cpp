#include <stdlib.h>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <mutex>
#include <chrono>
#include "Common.h"
#include "SSEServer.h"
#include "SSEClient.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "SSEEvent.h"
#include "SSEConfig.h"
#include "SSEChannel.h"
#include "InputSources/amqp/AmqpInputSource.h"

extern int stop;

using namespace std;

/**
  Constructor.
  @param config Pointer to SSEConfig object holding our configuration.
*/
SSEServer::SSEServer(SSEConfig *config) {
  _config = config;
  //stats.Init(_config, this);
}

/**
  Destructor.
*/
SSEServer::~SSEServer() {
  DLOG(INFO) << "SSEServer destructor called.";

  close(_serversocket);
  _client_workers.JoinAll();
}

int SSEServer::GetListeningSocket() {
  return _serversocket;
}

SSEClientWorker* SSEServer::GetClientWorker() {
  return NULL;
}

/**
  Broadcast event to channel.
  @param event Reference to SSEEvent to broadcast.
**/
bool SSEServer::Broadcast(SSEEvent& event) {
  SSEChannel* ch;
  const string& chName = event.getpath();

  ch = GetChannel(chName, _config->GetValueBool("server.allowUndefinedChannels"));
  if (ch == NULL) {
    LOG(ERROR) << "Discarding event recieved on invalid channel: " << chName;
    return false;
  }

  ch->BroadcastEvent(event);

  return true;
}

/**
  Checks if a client is allowed to publish to channel.
  @param client Pointer to SSEClient.
  @param chConf Reference to ChannelConfig object to check against.
**/
bool SSEServer::IsAllowedToPublish(SSEClient* client, const ChannelConfig& chConf) {
  if (chConf.allowedPublishers.size() < 1) return true;

  BOOST_FOREACH(const iprange_t& range, chConf.allowedPublishers) {
    if ((client->GetSockAddr() & range.mask) == (range.range & range.mask)) {
      LOG(INFO) << "Allowing publish to " << chConf.id << " from client " << client->GetIP();
      return true;
    }
  }

  DLOG(INFO) << "Dissallowing publish to " << chConf.id << " from client " << client->GetIP();
  return false;
}

/**
 Handle POST requests.
 @param client Pointer to SSEClient initiating the request.
 @param req Pointer to HTTPRequest.
 **/
void SSEServer::PostHandler(SSEClient* client, HTTPRequest* req) {
  SSEEvent event(req->GetPostData());
  bool validEvent;
  const string& chName = req->GetPath().substr(1);

  // Set the event path to the endpoint we recieved the POST on.
  event.setpath(chName);

  // Validate the event.
  validEvent = event.compile();

  // Check if channel exist.
  SSEChannel* ch = GetChannel(chName);

  if (ch == NULL) {
    // Handle creation of new channels.
    if (_config->GetValueBool("server.allowUndefinedChannels")) {
      if (!IsAllowedToPublish(client, _config->GetDefaultChannelConfig())) {
        HTTPResponse res(403);
        client->Send(res.Get());
        return;
      }

      if (!validEvent) {
        HTTPResponse res(400);
        client->Send(res.Get());
        return;
      }

      // Create the channel.
      ch = GetChannel(chName, true);
    } else {
      HTTPResponse res(404);
      client->Send(res.Get());
      return;
    }
  } else {
    // Handle existing channels.
    if (!IsAllowedToPublish(client, ch->GetConfig())) {
      HTTPResponse res(403);
      client->Send(res.Get());
      return;
    }

    if (!validEvent) {
      HTTPResponse res(400);
      client->Send(res.Get());
      return;
    }
  }

  // Broacast the event.
  Broadcast(event);

  // Event broadcasted OK.
  HTTPResponse res(200);
  client->Send(res.Get());
}

/**
  Start the server.
*/
void SSEServer::Run() {
  unsigned int i;
  InitSocket();

/*
  if (_config->GetValueBool("amqp.enabled")) {
      _datasource = boost::shared_ptr<SSEInputSource>(new AmqpInputSource());
      _datasource->Init(this);
      _datasource->Run();
  }

  InitChannels();
*/
  
  
  for (i = 0; i < 4; i++) {
    _client_workers.AddWorker(new SSEClientWorker(this));
  }

  _client_workers.StartWorkers();
  
  while(!stop) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  LOG(INFO) << "Exit.";
}

/**
  Initialize server socket.
*/
void SSEServer::InitSocket() {
  int on = 1;

  /* Ignore SIGPIPE. */
  signal(SIGPIPE, SIG_IGN);

  /* Set up listening socket. */
  _serversocket = socket(AF_INET, SOCK_STREAM, 0);
  LOG_IF(FATAL, _serversocket == -1) << "Error creating listening socket.";

  /* Reuse port and address. */
  setsockopt(_serversocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

  memset((char*)&_sin, '\0', sizeof(_sin));
  _sin.sin_family  = AF_INET;
  _sin.sin_port  = htons(_config->GetValueInt("server.port"));

  LOG_IF(FATAL, (bind(_serversocket, (struct sockaddr*)&_sin, sizeof(_sin))) == -1) <<
    "Could not bind server socket to " << _config->GetValue("server.bindip") << ":" << _config->GetValue("server.port");

  LOG_IF(FATAL, (listen(_serversocket, 0)) == -1) << "Call to listen() failed.";
  LOG_IF(FATAL, fcntl(_serversocket, F_SETFL, O_NONBLOCK) == -1) << "fcntl O_NONBLOCK on serversocket failed.";

  LOG(INFO) << "Listening on " << _config->GetValue("server.bindip")  << ":" << _config->GetValue("server.port");
}

/**
  Initialize static configured channels.
*/
void SSEServer::InitChannels() {
  BOOST_FOREACH(ChannelMap_t::value_type& chConf, _config->GetChannels()) {
    SSEChannel* ch = new SSEChannel(chConf.second, chConf.first);
    _channels.push_back(SSEChannelPtr(ch));
  }
}

/**
  Get instance pointer to SSEChannel object from id if it exists.
  @param The id/path of the channel you want to get a instance pointer to.
*/
SSEChannel* SSEServer::GetChannel(const string id, bool create) {
  SSEChannelList::iterator it;
  SSEChannel* ch = NULL;

  for (it = _channels.begin(); it != _channels.end(); it++) {
    SSEChannel* chan = static_cast<SSEChannel*>((*it).get());
    if (chan->GetId().compare(id) == 0) {
      return chan;
    }
  }

  if (create) {
    ch = new SSEChannel(_config->GetDefaultChannelConfig(), id);
    _channels.push_back(SSEChannelPtr(ch));
  }

  return ch;
}

/**
  Returns a const reference to the channel list.
*/
const SSEChannelList& SSEServer::GetChannelList() {
  return _channels;
}

/**
  Returns the SSEConfig object.
*/
SSEConfig* SSEServer::GetConfig() {
  return _config;
}
