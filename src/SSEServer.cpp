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

  close(_serversocket);

  /* TODO: Quit worker threads and close epoll filedescriptors. */
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

  if (_config->GetValueBool("amqp.enabled")) {
      _datasource = boost::shared_ptr<SSEInputSource>(new AmqpInputSource());
      _datasource->Init(this);
      _datasource->Run();
  }

  InitChannels();

  // Start client workers.
  for (i = 0; i < 4; i++) {
    boost::shared_ptr<worker_ctx_t> ctx(new worker_ctx_t());

    ctx->id = i;
  
    ctx->epoll_fd = epoll_create1(0);
    LOG_IF(FATAL, ctx->epoll_fd == -1) << "epoll_create1 failed for ClientWorker " << ctx->id;

    ctx->thread = boost::thread(&SSEServer::ClientWorker, this, ctx);

    _clientWorkers.push_back(ctx);
  }

  _curClientWorker = _clientWorkers.begin();

  // Start accept workers.
  for (i = 0; i < 4; i++) {
    boost::shared_ptr<worker_ctx_t> ctx(new worker_ctx_t());
    struct epoll_event event;

    ctx->id = i;
    ctx->epoll_fd = epoll_create1(0);

    LOG_IF(FATAL, ctx->epoll_fd == -1) << "epoll_create1 failed for AcceptWorker " << ctx->id;

    event.events = EPOLLIN | EPOLLEXCLUSIVE;
    event.data.ptr = NULL;

    LOG_IF(FATAL, epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, _serversocket, &event) == -1) << "Failed to add serversocket to epoll in AcceptWorker " << ctx->id;

    ctx->thread = boost::thread(&SSEServer::AcceptWorker, this, ctx);
    _acceptWorkers.push_back(ctx);
  }

  for (WorkerThreadList::iterator it = _acceptWorkers.begin(); it != _acceptWorkers.end(); it++) {
    (*it)->thread.join();
  }
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

/**
 Returns the epoll filedescriptor for a client worker.
 Will roundrobin through available workers.
*/
int SSEServer::GetClientWorkerRR() {
  boost::mutex::scoped_lock lock(_clientWorkerLock);

  if (_curClientWorker == _clientWorkers.end()) {
    _curClientWorker = _clientWorkers.begin();
  }

  LOG(INFO) << "curClientworker id: " << (*_curClientWorker)->id;
  return (*(_curClientWorker++))->epoll_fd;
}

/**
  Accept new client connections.
*/
void SSEServer::AcceptWorker(boost::shared_ptr<worker_ctx_t> ctx) {
  boost::shared_ptr<struct epoll_event[]> eventList(new struct epoll_event[MAXEVENTS]);

  DLOG(INFO) << "Started AcceptWorker " << ctx->id;

  while(!stop) {

    int n = epoll_wait(ctx->epoll_fd, eventList.get(), MAXEVENTS, -1);

    for (int i = 0; i < n; i++) {
      struct sockaddr_in csin;
      socklen_t clen;
      int tmpfd;

      memset((char*)&csin, '\0', sizeof(csin));
      clen = sizeof(csin);

      // Accept the connection.
      tmpfd = accept(_serversocket, (struct sockaddr*)&csin, &clen);

      if (tmpfd == -1) {
        switch (errno) {
          case EMFILE:
            LOG(ERROR) << "All connections available used. Cannot accept more connections.";
            usleep(100000);
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

      DLOG(INFO) << "Client accepted in AcceptWorker " << ctx->id;

      if (!client->AddToEpoll(GetClientWorkerRR())) {
        client->Destroy();
      }
    }
  }

  close(ctx->epoll_fd);
  DLOG(INFO) << "AcceptWorker " << ctx->id << " exited.";
}

/**
  Read request and route client to the requested channel.
*/
void SSEServer::ClientWorker(boost::shared_ptr<worker_ctx_t> ctx) {
  char buf[4096];
  boost::shared_ptr<struct epoll_event[]> eventList(new struct epoll_event[MAXEVENTS]);

  LOG(INFO) << "Started client router thread.";

  while(1) {
    int n = epoll_wait(ctx->epoll_fd, eventList.get(), MAXEVENTS, -1);
    LOG(INFO) << "Epoll event in " << ctx->id;
    for (int i = 0; i < n; i++) {
      SSEClient* client;
      client = static_cast<SSEClient*>(eventList[i].data.ptr);

      // Close socket if an error occurs.
      if (eventList[i].events & EPOLLERR) {
        DLOG(WARNING) << "Error occurred while reading data from client " << client->GetIP() << ".";
        client->Destroy();
        stats.router_read_errors++;
        continue;
      }

      if ((eventList[i].events & EPOLLHUP) || (eventList[i].events & EPOLLRDHUP)) {
        DLOG(WARNING) << "Client " << client->GetIP() << " hung up.";
        client->Destroy();
        continue;
      }

      if (eventList[i].events & EPOLLOUT) {
        // Send data present in send buffer,
        DLOG(INFO) << client->GetIP() << ": EPOLLOUT, flushing send buffer.";
        client->Flush();
        continue;
      }

      // Read from client.
      size_t len = client->Read(&buf, 4096);

      if (len <= 0) {
        stats.router_read_errors++;
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
         client->Destroy();
         stats.invalid_http_req++;
         continue;

        case HTTP_REQ_TO_BIG:
         client->Destroy();
         stats.oversized_http_req++;
         continue;

        case HTTP_REQ_OK: break;

        case HTTP_REQ_POST_INVALID_LENGTH:
          { HTTPResponse res(411, "", false); client->Send(res.Get()); }
          client->Destroy();
          continue;

        case HTTP_REQ_POST_TOO_LARGE:
          DLOG(INFO) << "Client " <<  client->GetIP() << " sent too much POST data.";
          { HTTPResponse res(413, "", false); client->Send(res.Get()); }
          client->Destroy();
          continue;

        case HTTP_REQ_POST_START:
          if (!_config->GetValueBool("server.enablePost")) {
            { HTTPResponse res(400, "", false); client->Send(res.Get()); }
            client->Destroy();
          } else {
            { HTTPResponse res(100, "", false); client->Send(res.Get()); }
          }
          continue;

        case HTTP_REQ_POST_INCOMPLETE: continue;

        case HTTP_REQ_POST_OK:
          if (_config->GetValueBool("server.enablePost")) {
            PostHandler(client, req);
          } else {
            { HTTPResponse res(400, "", false); client->Send(res.Get()); }
          }

          client->Destroy();
          continue;
      }

      if (!req->GetPath().empty()) {
        // Handle /stats endpoint.
        if (req->GetPath().compare("/stats") == 0) {
          stats.SendToClient(client);
          continue;
        } else if (req->GetPath().compare("/") == 0) {
          HTTPResponse res;
          res.SetBody("OK\n");
          client->Send(res.Get());
          client->Destroy();
          continue;
        }

        string chName = req->GetPath().substr(1);
        SSEChannel *ch = GetChannel(chName);

        DLOG(INFO) << "Channel: " << chName;

        if (ch != NULL) {
          /* We should handle the entire lifecycle of the client here.
             Handle EPOLLOUT.
          */
          epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, client->Getfd(), NULL);
          ch->AddClient(client, req);
        } else {
          HTTPResponse res;
          res.SetStatus(404);
          res.SetBody("Channel does not exist.\n");
          client->Send(res.Get());
          client->Destroy();
        }
      }
    }
  }
}
