#include "Common.h"
#include "SSEAcceptorWorker.h"
#include <memory>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

using namespace std;

SSEAcceptorWorker::SSEAcceptorWorker(SSEServer* server) {
  _server = server;
  _epoll_fd = epoll_create1(0);
}

SSEAcceptorWorker::~SSEAcceptorWorker() {
  if (_epoll_fd != -1) {
    close(_epoll_fd);
  }
}

void SSEAcceptorWorker::ThreadMain() {
  std::shared_ptr<struct epoll_event[]> eventList(new struct epoll_event[MAXEVENTS]);
  struct epoll_event event;

  DLOG(INFO) << "AcceptorWorker thread " << Id() << " started.";

  if (_epoll_fd == -1) {
    LOG(FATAL) << "epoll_create1() failed in SSEAcceptorWorker " << Id() << ": " << strerror(errno);
    return;
  }

  event.events = EPOLLIN | EPOLLEXCLUSIVE;
  event.data.ptr = NULL;

  LOG_IF(FATAL, epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server->GetListeningSocket(), &event) == -1) << "Failed to add serversocket to epoll in AcceptWorker " << Id();

  while(1) {
    int n = epoll_wait(_epoll_fd, eventList.get(), MAXEVENTS, -1);

    for (int i = 0; i < n; i++) {
      struct sockaddr_in csin;
      socklen_t clen;
      int client_fd;

      memset((char*)&csin, '\0', sizeof(csin));
      clen = sizeof(csin);

      // Accept the connection.
      client_fd = accept(_server->GetListeningSocket(), (struct sockaddr*)&csin, &clen);

      if (client_fd == -1) {
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
      fcntl(client_fd, F_SETFL, O_NONBLOCK);

      //_server->GetClientWorker()->NewClient(client_fd, &csin);

      DLOG(INFO) << "Client accepted in AcceptWorker " << Id();
    }
  }
}
