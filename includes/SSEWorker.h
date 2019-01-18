#include <memory>
#include <thread>
#include <vector>

class SSEWorkerPool;

class SSEWorker {
  friend class SSEWorkerPool;
  public:
    SSEWorker();
    ~SSEWorker();
    void Run();

  private:
    unsigned int _pool_id;
    std::thread _thread;
    virtual void ThreadMain();
};

class SSEWorkerPool {
  public:
    SSEWorkerPool();
    ~SSEWorkerPool();

    SSEWorker* AddWorker(SSEWorker* worker);

  private:
    std::vector<std::unique_ptr<SSEWorker>> _workers;
};