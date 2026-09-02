#pragma once

#include "threadpool.h"

#include <iostream>

namespace Threadpool {
template <typename F, typename... Args>
ThreadpoolStatusCode Threadpool::enqueue(F &&f, Args &&...args) {
  if (stop.load()) {
    std::cout << "Threadpool has been shutdown & is no longer valid."
              << std::endl;
    return ThreadpoolStatusCode::e_DEAD;
  }
  std::lock_guard lockQueue(drain_mutex);
  if (drained.load()) {
    std::cout << "Threadpool is draining and will not accept any new tasks."
              << std::endl;
    return ThreadpoolStatusCode::e_DRAINED;
  }
  {
    std::lock_guard lock(mutex);
    if (queueSize == threadPoolQueue.size()) {
      std::cout << "Threadpool is full." << std::endl;
      return ThreadpoolStatusCode::e_QUEUE_FULL;
    }
  }
  auto task = std::bind(f, args...);
  std::lock_guard lock(mutex);
  std::cout << "Adding task to the queue" << std::endl;
  threadPoolQueue.push(task);
  cv.notify_one();
  return ThreadpoolStatusCode::e_SUCCESS;
}
}; // namespace Threadpool
