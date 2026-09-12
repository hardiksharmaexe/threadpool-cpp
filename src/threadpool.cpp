#include "threadpool.h"

#include <format>
#include <iostream>

namespace Threadpool {
std::ostream &operator<<(std::ostream &os, ThreadpoolStatusCode code) {
  switch (code) {
  case ThreadpoolStatusCode::e_ACTIVE:
    os << "e_ACTIVE";
    break;
  case ThreadpoolStatusCode::e_DEAD:
    os << "e_DEAD";
    break;
  case ThreadpoolStatusCode::e_DRAINED:
    os << "e_DRAINED";
    break;
  case ThreadpoolStatusCode::e_QUEUE_FULL:
    os << "e_QUEUE_FULL";
    break;
  case ThreadpoolStatusCode::e_SUCCESS:
    os << "e_SUCCESS";
    break;
  default:
    os << "e_UNKNOWN";
    break;
  }
  return os;
}

void Threadpool::worker(int workerId) {
  std::cout << std::format("Starting Worker {}", workerId) << std::endl;
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock lock(mutex);
      cv.wait(lock, [&] { return stop.load() || !threadPoolQueue.empty(); });

      if (stop.load() && threadPoolQueue.empty()) {
        return;
      }

      task = threadPoolQueue.front();
      threadPoolQueue.pop();
    }
    std::cout << std::format("Worker {} is executing the task", workerId)
              << std::endl;
    cvDrain.notify_one();
    task();
    std::cout << std::format("Worker {} completed the execution", workerId)
              << std::endl;
  }
}

Threadpool::Threadpool(unsigned short int workerCount,
                       unsigned short int queueSize)
    : queueSize(queueSize) {
  std::cout << "Starting Threadpool" << std::endl;
  for (int i = 0; i < workerCount; ++i) {
    workers.emplace_back(&Threadpool::worker, this, i);
  }
}

Threadpool::~Threadpool() {
  shutdown();
  std::cout << "Destroying Threadpool" << std::endl;
}

ThreadpoolStatusCode Threadpool::shutdown() {
  if (stop.load()) {
    std::cout << "Threadpool is already shutdown" << std::endl;
    return ThreadpoolStatusCode::e_DEAD;
  }
  if (!drained.load()) {
    if (ThreadpoolStatusCode rc = drain();
        rc != ThreadpoolStatusCode::e_SUCCESS) {
      return rc;
    }
  }
  {
    std::lock_guard lock(mutex);
    stop.store(true);
  }

  cv.notify_all();

  for (auto &worker : workers) {
    worker.join();
  }
  std::cout << "Threadpool shutdown completed." << std::endl;
  return ThreadpoolStatusCode::e_SUCCESS;
}

ThreadpoolStatusCode Threadpool::status(int &pendingTasksCount) {
  std::lock_guard lock(mutex);
  pendingTasksCount = threadPoolQueue.size();
  std::cout << std::format("Tasks pending in the Threadpool: {}",
                           pendingTasksCount)
            << std::endl;
  if (stop.load()) {
    return ThreadpoolStatusCode::e_DEAD;
  }
  if (drained.load()) {
    return ThreadpoolStatusCode::e_DRAINED;
  }
  return ThreadpoolStatusCode::e_ACTIVE;
}

ThreadpoolStatusCode Threadpool::drain() {
  if (drained.load()) {
    std::cout << "Threadpool has already been drained." << std::endl;
    return ThreadpoolStatusCode::e_DRAINED;
  }
  std::lock_guard lockQueue(drain_mutex);
  std::unique_lock lock(mutex);
  cvDrain.wait(lock, [&] { return threadPoolQueue.empty(); });
  drained.store(true);
  std::cout << "Queue draining completed" << std::endl;
  return ThreadpoolStatusCode::e_SUCCESS;
}

void Threadpool::reset() {
  if (stop.load()) {
    std::cout << "Threadpool is dead. Can not reset." << std::endl;
    return;
  }
  if (!drained.load()) {
    std::cout << "Nothing to reset" << std::endl;
    return;
  }
  std::lock_guard lockQueue(drain_mutex);
  drained.store(false);
}

}; // namespace Threadpool
