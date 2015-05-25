#include "SSEServer.h"
#include <iostream>
#include <thread>

using namespace std;

SSEServer::SSEServer(SSEConfig *config) {
  this->config = config;
}

SSEServer::~SSEServer() {
  vector<SSEChannel*>::iterator it;

  DLOG(INFO) << "SSEServer destructor called.";

  pthread_cancel(routerThread);

  close(serverSocket);
  close(efd);
  free(eventList);

  for (it = channels.begin(); it != channels.end(); it++) {
    delete(*it);
  }
}

string SSEServer::getUri(const char* str) {
  string uri;

  if (strncmp("GET ", str, 4) == 0) {
    uri.insert(0, str, 4, string::npos);

    // Do not include data after space.
    uri = uri.substr(0, uri.find(" "));

    // Do not include newline.
    int nlpos = uri.find('\n');

    if (nlpos > 0) {
      uri.erase(nlpos);
    }
  
    return uri;
  }

  return "";
}

void SSEServer::run() {
  initSocket();
  initChannels();
  pthread_create(&routerThread, NULL, &SSEServer::routerThreadMain, this);
  acceptLoop();
}

void SSEServer::initSocket() {
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
  sin.sin_port  = htons(config->getServer().port);

  LOG_IF(FATAL, (bind(serverSocket, (struct sockaddr*)&sin, sizeof(sin))) == -1) <<
    "Could not bind server socket to " << config->getServer().bindIP << ":" << config->getServer().port;

  LOG_IF(FATAL, (listen(serverSocket, 0)) == -1) << "Call to listen() failed.";

  LOG(INFO) << "Listening on " << config->getServer().bindIP << ":" << config->getServer().port;

  efd = epoll_create1(0);
  LOG_IF(FATAL, efd == -1) << "epoll_create1 failed.";

  eventList = (struct epoll_event *)calloc(MAXEVENTS, sizeof(struct epoll_event));
  LOG_IF(FATAL, eventList == NULL) << "Could not allocate memory for epoll eventlist.";
}

void SSEServer::initChannels() {
  SSEChannelConfigList::iterator it;
  SSEChannelConfigList chanList = config->getChannels();

  for (it = chanList.begin(); it != chanList.end(); it++) {
    channels.push_back(new SSEChannel(*it));
  }
}

SSEChannel* SSEServer::getChannel(string id) {
  SSEChannelList::iterator it;

  for (it = channels.begin(); it != channels.end(); it++) {
    if (((SSEChannel*)*it)->getID().compare(id) == 0) {
      return (SSEChannel*)*it;
    }
  }

  return NULL;
}

void SSEServer::acceptLoop() {
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
    event.data.fd = tmpfd;
    event.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(efd, EPOLL_CTL_ADD, tmpfd, &event);

    if (ret == -1) {
      LOG(ERROR) << "Could not add client to epoll eventlist: " << strerror(errno);
      close(tmpfd);
      continue;
    }

    // Log client ip.
    char ip[32];
    inet_ntop(AF_INET, &csin.sin_addr, (char*)&ip, 32);
    LOG(INFO) << "Accepted new connection from " << ip;
  }
}

void *SSEServer::routerThreadMain(void *pThis) {
  SSEServer *srv = static_cast<SSEServer*>(pThis);
  srv->clientRouterLoop();
  return NULL;
}

/*
* Read request and route client to the requested channel.
*/
void SSEServer::clientRouterLoop() {
  int i, n;
  char buf[512];
  string uri;

  DLOG(INFO) << "Started client router thread.";

  while(1) {
    n = epoll_wait(efd, eventList, MAXEVENTS, -1);

    for (i = 0; i < n; i++) {
      /* Close socket if an error occurs. */
      if ((eventList[i].events & EPOLLERR) || (eventList[i].events & EPOLLHUP) || (!(eventList[i].events & EPOLLIN))) {
        DLOG(ERROR) << "Error occoured while reading data from socket.";
        close(eventList[i].data.fd);
        continue;
      }

      memset(buf, '\0', 512);

       /* Read from client. */
      int len = read(eventList[i].data.fd, &buf, 512);
      uri = getUri(buf);
      DLOG(INFO) << "Read " << len << " bytes from client: " << buf;

      if (!uri.empty()) {
        DLOG(INFO) << "CHANNEL:" << uri.substr(1) << ".";

        // substr(1) to remove the /.
        SSEChannel *ch = getChannel(uri.substr(1));
        if (ch != NULL) {
          ch->addClient(eventList[i].data.fd);
          epoll_ctl(efd, EPOLL_CTL_DEL, eventList[i].data.fd, NULL);
        } else {
          write(eventList[i].data.fd, "Channel does not exist.\r\n", 25);
          close(eventList[i].data.fd);
        }
      }
    }
  }
}