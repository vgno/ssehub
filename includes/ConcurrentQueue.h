#include <queue>
#include <boost/thread.hpp>

template<typename Data>
class ConcurrentQueue {
  private:
    std::queue<Data> _queue;
    mutable boost::mutex _mutex;
    boost::condition_variable _cond;

  public:
    void Push(Data const& data) {
      boost::mutex::scoped_lock lock(_mutex);
      _queue.push(data);
      lock.unlock();
      _cond.notify_one();
    }

    bool Empty() const {
      boost::mutex::scoped_lock lock(_mutex);
      return _queue.empty();
    }

    bool TryPop(Data& popped_value) {
      boost::mutex::scoped_lock lock(_mutex);
      if(_queue.empty()) {
        return false;
      }

      popped_value=_queue.front();
      _queue.pop();
      return true;
    }

    void WaitPop(Data& popped_value) {
      boost::mutex::scoped_lock lock(_mutex);
      while(_queue.empty()) {
        _cond.wait(lock);
      }

      popped_value=_queue.front();
      _queue.pop();
    }
};
