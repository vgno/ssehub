#ifndef SSEWORKER_H
#define SSEWORKER_H

#include <memory>
#include <thread>
#include <vector>

template <class T>
using SSEWorkerList = std::vector<std::shared_ptr<T>>;

class SSEWorker {
  public:
    SSEWorker() {}
    ~SSEWorker() {}

    void Run() {
      if (!_thread.joinable()) {
        _thread = std::thread(&SSEWorker::ThreadMain, this);
      }
    }

    std::thread& Thread() {
      return _thread;
    }

    std::thread::id Id() {
      return _thread.get_id();
    }

  private:
    std::thread _thread;

  protected:
    virtual void ThreadMain() {}
};

template <class T>
class SSEWorkerGroup {
  public:
    SSEWorkerGroup<T>() {};
    ~SSEWorkerGroup<T>(){};

    void AddWorker(T* worker) {
       _workers.push_back(std::shared_ptr<T>(worker));
    }

    void StartWorkers() {
      for (auto &wrk :_workers) {
        wrk->Run();
      }
    }

    void JoinAll() {
      for (auto &wrk : _workers) {
        if (wrk->Thread().joinable()) {
          wrk->Thread().join();
        }
      }
    }

    SSEWorkerList<T>& GetWorkerList() {
      return _workers;
    }

  private:
    SSEWorkerList<T> _workers;
};

#endif
