#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Threadpool {
class Threadpool {
private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> threadPoolQueue;
  std::mutex mutex, drain_mutex;
  bool stop = false;
  bool drained = false;
  std::condition_variable cv, cvDrain;

  void worker(int workerId);

public:
  explicit Threadpool(int workerCount = 10);
  ~Threadpool();
  template <typename F, typename... Args> void enqueue(F &&f, Args &&...args);
  void shutdown();
  void drain();
  void reset();
  void status(int &status);

  Threadpool(Threadpool &) = delete;
  Threadpool(const Threadpool &) = delete;
  Threadpool &operator=(Threadpool &&) = delete;
  Threadpool &operator=(const Threadpool &) = delete;
};
}; // namespace Threadpool

#include "threadpool.tpp"
