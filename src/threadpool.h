#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Threadpool {
enum class ThreadpoolStatusCode {
  e_SUCCESS,
  e_DEAD,
  e_DRAINED,
  e_ACTIVE,
  e_QUEUE_FULL,
};

std::ostream &operator<<(std::ostream &os, ThreadpoolStatusCode code);

class Threadpool {
private:
  std::vector<std::thread> workers;
  std::mutex mutex, drain_mutex;
  std::condition_variable cv, cvDrain;
  std::queue<std::function<void()>> threadPoolQueue;
  std::atomic<bool> drained{false};
  std::atomic<bool> stop{false};
  unsigned short int queueSize;

  void worker(int workerId);

public:
  explicit Threadpool(unsigned short int workerCount = 10,
                      unsigned short int queueSize = 1000);
  ~Threadpool();
  template <typename F, typename... Args>
  ThreadpoolStatusCode enqueue(F &&f, Args &&...args);
  ThreadpoolStatusCode shutdown();
  ThreadpoolStatusCode drain();
  void reset();
  ThreadpoolStatusCode status(int &pendingTasksCount);

  Threadpool(Threadpool &) = delete;
  Threadpool(const Threadpool &) = delete;
  Threadpool &operator=(Threadpool &&) = delete;
  Threadpool &operator=(const Threadpool &) = delete;
};
}; // namespace Threadpool

#include "threadpool.tpp"
