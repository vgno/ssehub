#include <memory>
#include "SSEWorker.h"

using namespace std;

SSEWorker::SSEWorker() {

}

SSEWorker::~SSEWorker() {

}

void SSEWorker::Run() {
  if (!_thread.joinable()) {
    _thread = std::thread(&SSEWorker::ThreadMain, this);
  }
}

void SSEWorker::ThreadMain() {

}

SSEWorkerPool::SSEWorkerPool() {

}

SSEWorkerPool::~SSEWorkerPool() {

}

SSEWorker* SSEWorkerPool::AddWorker(SSEWorker* worker) {
  _workers.push_back(unique_ptr<SSEWorker>(worker));
  return worker;
}