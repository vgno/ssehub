#include <stdlib.h>
#include "SSEServer.h"
#include "SSEClient.h"
#include "HTTPRequest.h"
#include "SSEEvent.h"
#include "SSEConfig.h"
#include "SSEChannel.h"
#include <boost/foreach.hpp>

using namespace std;

/**
  Constructor.
  @param config Pointer to SSEConfig object holding our configuration.
*/
SSEServer::SSEServer(SSEConfig *config) {
  this->config = config;
  stats.Init(config, this);
}

/**
  Destructor.
*/
SSEServer::~SSEServer() {
  DLOG(INFO) << "SSEServer destructor called.";

  pthread_cancel(routerThread);
  close(serverSocket);
  close(efd);
}

/**
  Wrapper function for AmqpCallback to satisfy the function-type pthread expects.
  @param pThis pointer to SSEServer instance.
  @param key AMQP routingkey,
  @param msg AMQP message.
*/
void SSEServer::AmqpCallbackWrapper(void* pThis, const string key, const string msg) {
  SSEServer* pt = static_cast<SSEServer *>(pThis);
  pt->AmqpCallback(key, msg);
}

/**
  AMQP callback function that will be called when a message arrives.
  @param key AMQP routingkey,
  @param msg AMQP message.
*/
void SSEServer::AmqpCallback(string key, string msg) {
  SSEEvent* event = new SSEEvent(msg);

  if (!event->compile()) {
    DLOG(ERROR) << "Discarding event with invalid format recieved on " << key;
    return;
  }

  string chName;
  // If path is set in the JSON event data use that as target channel name.
  if (!event->getpath().empty()) { chName = event->getpath(); }
  // Otherwise use the routingkey.
  else if (!key.empty())         { chName = key; }
  // If none of the is present just return and ignore the message.
  else                           { return; }

  SSEChannel *ch = GetChannel(chName);
  
  if (ch == NULL) {
    if (!config->GetValueBool("server.allowUndefinedChannels")) {
        DLOG(ERROR) << "Discarding event recieved on invalid channel: " << chName;
        return;
    }

    ch = new SSEChannel(config->GetDefaultChannelConfig(), chName);
    channels.push_back(SSEChannelPtr(ch));
  }

  ch->BroadcastEvent(event);
}

/**
  Start the server.
*/
void SSEServer::Run() {
  InitSocket();

  amqp.Start(config->GetValue("amqp.host"), config->GetValueInt("amqp.port"), 
      config->GetValue("amqp.user"),config->GetValue("amqp.password"), 
      config->GetValue("amqp.exchange"), "", 
      SSEServer::AmqpCallbackWrapper, this);

  InitChannels();

  pthread_create(&routerThread, NULL, &SSEServer::RouterThreadMain, this);
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
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  LOG_IF(FATAL, serverSocket == -1) << "Error creating listening socket.";

  /* Reuse port and address. */
  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

  memset((char*)&sin, '\0', sizeof(sin));
  sin.sin_family  = AF_INET;
  sin.sin_port  = htons(config->GetValueInt("server.port"));

  LOG_IF(FATAL, (bind(serverSocket, (struct sockaddr*)&sin, sizeof(sin))) == -1) <<
    "Could not bind server socket to " << config->GetValue("server.bindip") << ":" << config->GetValue("server.port");

  LOG_IF(FATAL, (listen(serverSocket, 0)) == -1) << "Call to listen() failed.";

  LOG(INFO) << "Listening on " << config->GetValue("server.bindip")  << ":" << config->GetValue("server.port");

  efd = epoll_create1(0);
  LOG_IF(FATAL, efd == -1) << "epoll_create1 failed.";
}

/**
  Initialize static configured channels.
*/
void SSEServer::InitChannels() {
  BOOST_FOREACH(ChannelMap_t::value_type& chConf, config->GetChannels()) {
    SSEChannel* ch = new SSEChannel(chConf.second, chConf.first);
    channels.push_back(SSEChannelPtr(ch));
  }
}

/**
  Get instance pointer to SSEChannel object from id if it exists.
  @param The id/path of the channel you want to get a instance pointer to.
*/
SSEChannel* SSEServer::GetChannel(const string id) {
  SSEChannelList::iterator it;

  for (it = channels.begin(); it != channels.end(); it++) {
    SSEChannel* chan = static_cast<SSEChannel*>((*it).get());
    if (chan->GetId().compare(id) == 0) {
      return chan;
    }
  }

  return NULL;
}

/**
  Returns an iterator object pointing to the first element in the channels list.
*/
SSEChannelList::const_iterator SSEServer::ChannelsBegin() {
  return channels.begin();
}

/**
  Returns an iterator object pointing to the last element in the channels list.
*/
SSEChannelList::const_iterator SSEServer::ChannelsEnd() {
  return channels.end();
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
    tmpfd = accept(serverSocket, (struct sockaddr*)&csin, &clen);

    /* Got an error ? Handle it. */
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
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = new SSEClient(tmpfd, &csin);

    int ret = epoll_ctl(efd, EPOLL_CTL_ADD, tmpfd, &event);
    if (ret == -1) {
      LOG(ERROR) << "Could not add client to epoll eventlist: " << strerror(errno);
      static_cast<SSEClient*>(event.data.ptr)->Destroy();
      continue;
    }
  }
}

/**
  Wrapper function for ClientRouterLoop to satisfy the function-type pthread expects.
  @param pThis Pointer to the running SSEServer instance.
*/
void *SSEServer::RouterThreadMain(void *pThis) {
  SSEServer *srv = static_cast<SSEServer*>(pThis);
  srv->ClientRouterLoop();
  return NULL;
}

/**
  Read request and route client to the requested channel.
*/
void SSEServer::ClientRouterLoop() {
  char buf[4096];
  boost::shared_ptr<struct epoll_event[]> eventList(new struct epoll_event[MAXEVENTS]);
  
  DLOG(INFO) << "Started client router thread.";

  while(1) {
    int n = epoll_wait(efd, eventList.get(), MAXEVENTS, -1);

    for (int i = 0; i < n; i++) {
      SSEClient* client;
      client = static_cast<SSEClient*>(eventList[i].data.ptr);

      // Close socket if an error occurs.
      if ((eventList[i].events & EPOLLERR) || (eventList[i].events & EPOLLHUP) || (!(eventList[i].events & EPOLLIN))) {
        DLOG(ERROR) << "Error occoured while reading data from socket.";
        client->Destroy();
        continue;
      }

      // Read from client.
      size_t len = client->Read(&buf, 4096);
      buf[len] = '\0';

      // Parse the request.
      HTTPRequest req(buf, len);

      DLOG(INFO) << "Read " << len << " bytes from client: " << buf;

      if (!req.Success()) {
        LOG(INFO) << "Invalid HTTP request receieved, shutting down connection.";
        client->Send("Invalid HTTP request receieved, shutting down connection.\r\n");
        client->Destroy();
        continue;
      }

      if (!req.GetPath().empty()) {
        // Handle /stats endpoint.
        if (req.GetPath().compare("/stats") == 0) {
          stats.SendToClient(client);
          client->Destroy();
          continue;
        }

        string chName = req.GetPath().substr(1);
        DLOG(INFO) << "CHANNEL:" << chName << ".";
        
        // substr(1) to remove the /.
        SSEChannel *ch = GetChannel(chName);
        if (ch != NULL) {
          ch->AddClient(client, &req);
          epoll_ctl(efd, EPOLL_CTL_DEL, client->Getfd(), NULL);
        } else {
          client->Send("Channel does not exist.\r\n");
          client->Destroy();
        }
      }
    }
  }
}
