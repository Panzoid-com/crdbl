#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ConcurrentQueue {
public:
  ConcurrentQueue()
    : is_done(false)
  {}

  ~ConcurrentQueue() {
    is_done = true;
    cv.notify_all();
  }

  void enqueue(T && value) {
    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      data_queue.push(std::move(value));
    }
    cv.notify_one();
  }

  bool wait_dequeue(std::function<void(T &)> process_value) {
    std::queue<T> localQueue;
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      cv.wait(lock, [this] { return !data_queue.empty() || is_done; });
      localQueue.swap(data_queue);
    }

    while (!localQueue.empty()) {
      T value = std::move(localQueue.front());
      localQueue.pop();
      process_value(value);
    }

    return is_done;
  }

private:
  std::atomic<bool> is_done;
  std::queue<T> data_queue;
  std::mutex queue_mutex;
  std::condition_variable cv;
};
