#include <stdlib.h>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include "Common.h"
#include "SSEServer.h"
#include "SSEClient.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "SSEEvent.h"
#include "SSEConfig.h"
#include "SSEChannel.h"
#include "InputSources/amqp/AmqpInputSource.h"

using namespace std;

/**
  Constructor.
  @param config Pointer to SSEConfig object holding our configuration.
*/
SSEServer::SSEServer(SSEConfig *config) {
  _config = config;
  stats.Init(_config, this);
}

/**
  Destructor.
*/
SSEServer::~SSEServer() {
  DLOG(INFO) << "SSEServer destructor called.";

  pthread_cancel(_routerthread.native_handle());
  close(_serversocket);
  close(_efd);
}

bool SSEServer::Broadcast(SSEEvent& event) {
  SSEChannel* ch;
  const string& chName = event.getpath();
  
  ch = GetChannel(chName, _config->GetValueBool("server.allowUndefinedChannels"));
  if (ch == NULL) {
    LOG(ERROR) << "Discarding event recieved on invalid channel: " << chName;
    return false;
  }

  ch->BroadcastEvent(&event);

  return true;
}

/**
 Handle POST requests.
 @param client Pointer to SSEClient initiating the request.
 @param req Pointer to HTTPRequest.
 **/
void SSEServer::PostHandler(SSEClient* client, HTTPRequest* req) {
  SSEEvent event(req->GetPostData());

  const string& chName = req->GetPath().substr(1); 

  // Check if channel exist.
  SSEChannel* ch = GetChannel(chName, 
      _config->GetValueBool("server.allowUndefinedChannels"));

  if (ch == NULL) {
   HTTPResponse res(404);
   client->Send(res.Get());
   return; 
  }

  // Set the event path to the endpoint we recieved the POST on.
  event.setpath(chName);

  // Client unauthorized.
  if (!ch->IsAllowedToPublish(client)) {
    HTTPResponse res(403);
    client->Send(res.Get());
    return;
  }
 
  // Invalid format. 
  if (!event.compile()) {
    HTTPResponse res(400);
    client->Send(res.Get());
    return;
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
  InitSocket();

  _datasource = boost::shared_ptr<SSEInputSource>(new AmqpInputSource());
  _datasource->Init(this);
  _datasource->Run();

  InitChannels();

  _routerthread = boost::thread(&SSEServer::ClientRouterLoop, this);
  AcceptLoop();
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

  LOG(INFO) << "Listening on " << _config->GetValue("server.bindip")  << ":" << _config->GetValue("server.port");

  _efd = epoll_create1(0);
  LOG_IF(FATAL, _efd == -1) << "epoll_create1 failed.";
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
SSEChannel* SSEServer::GetChannel(const string id, bool create=false) {
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

/**
  Accept new client connections.
*/
void SSEServer::AcceptLoop() {
  while(!stop) {
    struct sockaddr_in csin;
    socklen_t clen;
    int tmpfd;

    memset((char*)&csin, '\0', sizeof(csin));
    clen = sizeof(csin);

    // Accept the connection.
    tmpfd = accept(_serversocket, (struct sockaddr*)&csin, &clen);

    /* Got an error ? Handle it. */
    if (tmpfd == -1) {
      switch (errno) {
        case EMFILE:
          LOG(ERROR) << "All connections available used. Exiting.";
          // As a safety measure exit when we have no more filehandles available on the system.
          // This is a sign that something is wrong and we are better off handling it this way, atleast for now.
          exit(1);
        break;

        default:
          LOG_IF(ERROR, !stop) << "Error in accept(): " << strerror(errno);
      }

      continue; /* Try again. */
    }

    // Set non-blocking on client socket.
    fcntl(tmpfd, F_SETFL, O_NONBLOCK);

    // Add it to our epoll eventlist.
    SSEClient* client = new SSEClient(tmpfd, &csin);

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    event.data.ptr = static_cast<SSEClient*>(client);

    int ret = epoll_ctl(_efd, EPOLL_CTL_ADD, tmpfd, &event);
    if (ret == -1) {
      LOG(ERROR) << "Could not add client to epoll eventlist: " << strerror(errno);
      client->Destroy();
      continue;
    }
  }
}


/**
 Remove socket from epoll fd set.
 @param fd File descriptor to remove.
**/
void SSEServer::RemoveSocket(int fd) {
  epoll_ctl(_efd, EPOLL_CTL_DEL, fd, NULL);
}

/**
  Read request and route client to the requested channel.
*/
void SSEServer::ClientRouterLoop() {
  char buf[4096];
  boost::shared_ptr<struct epoll_event[]> eventList(new struct epoll_event[MAXEVENTS]);

  LOG(INFO) << "Started client router thread.";

  while(1) {
    int n = epoll_wait(_efd, eventList.get(), MAXEVENTS, -1);

    for (int i = 0; i < n; i++) {
      SSEClient* client;
      client = static_cast<SSEClient*>(eventList[i].data.ptr);

      // Close socket if an error occurs.
      if (eventList[i].events & EPOLLERR) {
        DLOG(WARNING) << "Error occurred while reading data from client " << client->GetIP() << ".";
        RemoveSocket(client->Getfd());
        client->Destroy();
        stats.router_read_errors++;
        continue;
      }

      if ((eventList[i].events & EPOLLHUP) || (eventList[i].events & EPOLLRDHUP)) {
        DLOG(WARNING) << "Client " << client->GetIP() << " hung up in router thread.";
        RemoveSocket(client->Getfd());
        client->Destroy();
        continue;
      }

      // Read from client.
      size_t len = client->Read(&buf, 4096);

      if (len <= 0) {
        stats.router_read_errors++;
        RemoveSocket(client->Getfd());
        client->Destroy();
        continue;
      }

      buf[len] = '\0';

      // Parse the request.
      HTTPRequest* req = client->GetHttpReq();
      HttpReqStatus reqRet = req->Parse(buf, len);

      switch(reqRet) {
        case HTTP_REQ_INCOMPLETE: continue;

        case HTTP_REQ_FAILED:
         RemoveSocket(client->Getfd());
         client->Destroy();
         stats.invalid_http_req++;
         continue;

        case HTTP_REQ_TO_BIG:
         RemoveSocket(client->Getfd());
         client->Destroy();
         stats.oversized_http_req++;
         continue;

        case HTTP_REQ_OK: break;

        case HTTP_REQ_POST_INVALID_LENGTH:
          { HTTPResponse res(411, "", false); client->Send(res.Get()); }
          RemoveSocket(client->Getfd());
          client->Destroy();
          continue;

        case HTTP_REQ_POST_TOO_LARGE:
          DLOG(INFO) << "Client " <<  client->GetIP() << " sent too much POST data.";
          { HTTPResponse res(413, "", false); client->Send(res.Get()); }
          RemoveSocket(client->Getfd());
          client->Destroy();
          continue;

        case HTTP_REQ_POST_START:
          { HTTPResponse res(100, "", false); client->Send(res.Get()); }
          continue;

        case HTTP_REQ_POST_INCOMPLETE: continue;

        case HTTP_REQ_POST_OK:
          RemoveSocket(client->Getfd());
          PostHandler(client, req);
          client->Destroy();
          continue;
      } 

      if (!req->GetPath().empty()) {
        // Handle /stats endpoint.
        if (req->GetPath().compare("/stats") == 0) {
          stats.SendToClient(client);
          continue;
        }

        string chName = req->GetPath().substr(1);
        SSEChannel *ch = GetChannel(chName);

        DLOG(INFO) << "Channel: " << chName;

        if (ch != NULL) {
          RemoveSocket(client->Getfd());
          ch->AddClient(client, req);
        } else {
          HTTPResponse res;
          res.SetBody("Channel does not exist.\n");
          client->Send(res.Get());
          RemoveSocket(client->Getfd());
          client->Destroy();
        }
      }
    }
  }
}
