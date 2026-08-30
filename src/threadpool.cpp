#include "threadpool.h"

#include <format>
#include <iostream>

namespace Threadpool {
void Threadpool::worker(int workerId) {
  std::cout << std::format("Starting Worker {}", workerId) << std::endl;
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock lock(mutex);
      cv.wait(lock, [&]() { return stop || !threadPoolQueue.empty(); });

      if (stop && threadPoolQueue.empty()) {
        return;
      }

      task = threadPoolQueue.front();
      threadPoolQueue.pop();
    }
    std::cout << std::format("Worker {} is executing the task", workerId)
              << std::endl;
    task();
    std::cout << std::format("Worker {} completed the execution", workerId)
              << std::endl;
    cvDrain.notify_one();
  }
}

Threadpool::Threadpool(int workerCount) {
  std::cout << "Starting Threadpool" << std::endl;
  for (int i = 0; i < workerCount; ++i) {
    workers.emplace_back(&Threadpool::worker, this, i);
  }
}

Threadpool::~Threadpool() {
  shutdown();
  std::cout << "Destroying Threadpool" << std::endl;
}

void Threadpool::shutdown() {
  drain();
  {
    std::unique_lock lock(mutex);
    stop = true;
  }

  cv.notify_all();

  for (auto &worker : workers) {
    worker.join();
  }
}

void Threadpool::status(int &status) {
  std::unique_lock lock(mutex);
  status = threadPoolQueue.size();
  std::cout << std::format("Tasks pending in the Threadpool: {}", status)
            << std::endl;
}

void Threadpool::drain() {
  std::unique_lock lockQueue(drain_mutex);
  std::unique_lock lock(mutex);
  cvDrain.wait(lock, [&] { return threadPoolQueue.empty(); });
  drained = true;
  std::cout << "Draining completed" << std::endl;
}

void Threadpool::reset() {
  std::unique_lock lockQueue(drain_mutex);
  drained = false;
}

}; // namespace Threadpool
