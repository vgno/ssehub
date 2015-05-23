#include "SSEServer.h"
#include <iostream>

using namespace std;

SSEServer::SSEServer(SSEConfig *config) {
  this->config = config;
}

SSEServer::~SSEServer() {
  vector<SSEChannel*>::iterator it;

  DLOG(INFO) << "SSEServer destructor called.";

  close(serverSocket);

  for (it = channels.begin(); it != channels.end(); it++) {
    free(*it);
  }
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
}

void SSEServer::initChannels() {
  SSEChannelConfigList::iterator it;
  SSEChannelConfigList chanList = config->getChannels();

  for (it = chanList.begin(); it != chanList.end(); it++) {
    channels.push_back(new SSEChannel(*it));
  }
}

void SSEServer::acceptLoop() {
  while(1) {
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
          LOG(ERROR) << "Error in accept(): " << strerror(errno);
      }

      continue; /* Try again. */
    }
    close(tmpfd);
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
  DLOG(INFO) << "Started client router thread.";
}