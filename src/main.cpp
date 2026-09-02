#include "threadpool.h"

#include <iostream>

void add(int x, int y) { std::cout << "Function call works too" << std::endl; }

int main() {
  Threadpool::Threadpool threadpool(5, 1);
  threadpool.drain();
  if (auto rc =
          threadpool.enqueue([] { std::cout << "Adding 1+3" << std::endl; });
      rc != Threadpool::ThreadpoolStatusCode::e_SUCCESS) {
    std::cout << "Unable to enqueue :( : " << rc << std::endl;
  }
  threadpool.drain();
  threadpool.enqueue(add, 3, 4);
  threadpool.enqueue(add, 3, 4);
  threadpool.reset();
  threadpool.enqueue(add, 3, 4);
  threadpool.enqueue(add, 3, 4);
  int pendingTasksCount;
  threadpool.status(pendingTasksCount);
  threadpool.shutdown();
  threadpool.enqueue(add, 3, 4);
  threadpool.enqueue(add, 3, 4);
  threadpool.drain();
  return 0;
}
