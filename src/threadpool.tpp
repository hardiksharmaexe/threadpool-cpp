#pragma once

#include "threadpool.h"

#include <iostream>

namespace Threadpool {
template <typename F, typename... Args>
void Threadpool::enqueue(F &&f, Args &&...args) {
  std::unique_lock lockQueue(drain_mutex);
  if (drained) {
    std::cout << "Threadpool is draining and will not accept any new tasks." << std::endl;
    return;
  }
  auto task = std::bind(f, args...);
  std::unique_lock lock(mutex);
  std::cout << "Adding task to the queue" << std::endl;
  threadPoolQueue.push(task);
  cv.notify_one();
}
}; // namespace Threadpool
