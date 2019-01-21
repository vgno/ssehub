#include <memory>
#include <mutex>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "SSEClientWorker.h"
#include "Common.h"
#include "SSEClient.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"

using namespace std;

SSEClientWorker::SSEClientWorker(SSEServer* server) {
  _server = server;
  _epoll_fd = epoll_create1(0);

  _client_list.reserve(50000);
  _client_list.max_load_factor(0.25);
}

SSEClientWorker::~SSEClientWorker() {
  if (_epoll_fd != -1) {
    close(_epoll_fd);
  }
}

void SSEClientWorker::_acceptClient() {
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
        break;

        default:
          LOG(ERROR) << "Error in accepting new client: " << strerror(errno);
      }

      return;
    }

  // Create the client object and add it to our client list.
  _newClient(client_fd, &csin);

  DLOG(INFO) << "Client accepted in worker " << Id();
}

void SSEClientWorker::_newClient(int fd, struct sockaddr_in* csin) {
  std::lock_guard<std::mutex> guard(_client_list_mutex);
  auto client = make_shared<SSEClient>(fd, csin);

  int ret = client->AddToEpoll(_epoll_fd, (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR));
  if (ret == -1) {
    LOG(WARNING) << "Could not add client to epoll: " << strerror(errno);
    return;
  }

  _client_list.emplace(make_pair(fd, client));
}

void SSEClientWorker::_read(std::shared_ptr<SSEClient> client, ClientList::iterator it) {
  char buf[1];

  // Read from client.
  size_t len = client->Read(&buf, 1);

  if (len <= 0) {
    _client_list.erase(it);
  }

  buf[len] = '\0';

  LOG(INFO) << "Read: " << buf;

  // Parse the request.
  HTTPRequest* req = client->GetHttpReq();
  HttpReqStatus reqRet = req->Parse(buf, len);

  switch(reqRet) {
    case HTTP_REQ_INCOMPLETE: return;

    case HTTP_REQ_FAILED:
      return;

    case HTTP_REQ_TO_BIG:
      return;

    case HTTP_REQ_OK: break;

    case HTTP_REQ_POST_INVALID_LENGTH:
      { HTTPResponse res(411, "", false); client->Write(res.Get()); }
      return;

    case HTTP_REQ_POST_TOO_LARGE:
      DLOG(INFO) << "Client " <<  client->GetIP() << " sent too much POST data.";
      { HTTPResponse res(413, "", false); client->Write(res.Get()); }
      return;

    case HTTP_REQ_POST_START:
      return;

    case HTTP_REQ_POST_INCOMPLETE: return;

    case HTTP_REQ_POST_OK:
      return;
  }

  HTTPResponse res;
  res.SetStatus(200);

  string s;
  for (int i = 0; i < 1000000; i++) {
    s.append(".");
  }

  res.SetBody(s + "\nYour request was: " + req->GetMethod() + " " + req->GetPath() + ". Num clients: " + std::to_string(_client_list.size()) + "\n");
  client->Write(res.Get());
  //shutdown(client->Getfd(), SHUT_RDWR);
}

void SSEClientWorker::ThreadMain() {
  std::shared_ptr<struct epoll_event[]> event_list(new struct epoll_event[MAXEVENTS]);
  struct epoll_event server_socket_event;

  DLOG(INFO) << "Worker thread " << Id() << " started.";

  if (_epoll_fd == -1) {
    LOG(FATAL) << "epoll_create1() failed in worker " << Id() << ": " << strerror(errno);
    return;
  }

  // Add server listening socket to epoll.
  server_socket_event.events = EPOLLIN | EPOLLEXCLUSIVE;
  server_socket_event.data.fd = _server->GetListeningSocket();
  LOG_IF(FATAL, epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server->GetListeningSocket(), &server_socket_event) == -1) << "Failed to add serversocket to epoll in AcceptWorker " << Id();

  while(!StopRequested()) {
    int n = epoll_wait(_epoll_fd, event_list.get(), MAXEVENTS, 1000);

    for (int i = 0; i < n; i++) {
      // Handle new connections.
      if (event_list[i].data.fd == _server->GetListeningSocket()) {
        LOG(INFO) << "Event on server socket.";
        if (event_list[i].events & EPOLLIN) {
          _acceptClient();
        }

        continue;
      }

      auto client_it = _client_list.find(event_list[i].data.fd);
      if (client_it == _client_list.end()) {
        LOG(ERROR) << "ERROR: Received event on filedescriptor which is not present in client list.";
        continue;
      }

      shared_ptr<SSEClient> client = client_it->second;

      // Close socket if an error occurs.
      if (event_list[i].events & EPOLLERR) {
        DLOG(WARNING) << "Error occurred while reading data from client " << client->GetIP() << ".";
        _client_list.erase(client_it);
        continue;
      }

      if ((event_list[i].events & EPOLLHUP) || (event_list[i].events & EPOLLRDHUP)) {
        DLOG(WARNING) << "Client " << client->GetIP() << " hung up.";
         _client_list.erase(client_it);
        continue;
      }

      if (event_list[i].events & EPOLLOUT) {
        DLOG(INFO) << client->GetIP() << ": EPOLLOUT, flushing send buffer.";
        client->Flush();
        continue;
      }

      _read(client, client_it);
    }
  }

  DLOG(INFO) << "Worker " << Id() << " stopped.";
}