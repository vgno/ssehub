#ifndef SSEWORKER_H
#define SSEWORKER_H

#include <memory>
#include <thread>
#include <vector>
#include <future>
#include <chrono>


template <class T>
using SSEWorkerList = std::vector<std::shared_ptr<T>>;

class SSEWorker {
  public:
    SSEWorker() {
      _stop_requested_future = _stop_requested.get_future();
    }

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

    bool StopRequested() {
      if (_stop_requested_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout) {
        return false;
      }

      return true;
    }

    void Stop() {
      _stop_requested.set_value();
    }

  private:
    std::thread _thread;
    std::promise<void> _stop_requested;
    std::future<void> _stop_requested_future;

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
          wrk->Stop();
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
