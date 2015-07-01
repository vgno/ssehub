#include <vector>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <pthread.h>
#include "SSEServer.h"
#include "SSEConfig.h"
#include "SSEInputSource.h"

using namespace std;

SSEInputSource::SSEInputSource() {}
SSEInputSource::~SSEInputSource() {}

void SSEInputSource::Init(SSEServer* server, DataSourceCallback callback) {
  _server = server;
  _config = server->GetConfig();
  _callback = callback;
}

void SSEInputSource::Run() {
  _thread = boost::thread(boost::bind(&SSEInputSource::Start, this));
}

void SSEInputSource::KillThread() {
  pthread_cancel(_thread.native_handle());
}
