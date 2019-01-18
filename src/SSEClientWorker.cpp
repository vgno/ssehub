#include <memory>
#include <mutex>
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

using namespace std;

SSEClientWorker::SSEClientWorker(SSEServer* server) {
  _epoll_fd = epoll_create1(0);
}

SSEClientWorker::~SSEClientWorker() {
  if (_epoll_fd != -1) {
    close(_epoll_fd);
  }
}

void SSEClientWorker::NewClient(int fd, struct sockaddr_in* csin) {
  std::lock_guard<std::mutex> guard(_client_list_mutex);
  auto client = std::make_shared<SSEClient>(fd, csin);

  if (!client->AddToEpoll(_epoll_fd)) {
    LOG(WARNING) << "Could not add client " << client->GetIP() << " to epoll: " << strerror(errno);
    return;
  }

  _client_list.push_back(client);
}

void SSEClientWorker::ThreadMain() {
  DLOG(INFO) << "ClientWorker thread " << Id() << " started." << endl;

  if (_epoll_fd == -1) {
    LOG(FATAL) << "epoll_create1() failed in SSEClientWorker " << Id() << ": " << strerror(errno);
    return;
  }

  /*
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
        continue;
      }

      if ((eventList[i].events & EPOLLHUP) || (eventList[i].events & EPOLLRDHUP)) {
        DLOG(WARNING) << "Client " << client->GetIP() << " hung up.";
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
          // We should handle the entire lifecycle of the client here.
          // Handle EPOLLOUT.
          
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
  */
}
