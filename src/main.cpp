#include "threadpool.h"

#include <iostream>

void add(int x, int y) { std::cout << "Function call works too" << std::endl; }

int main() {
  Threadpool::Threadpool threadpool(5);
  threadpool.enqueue([]{ std::cout << "Adding 1+3" << std::endl; });
  threadpool.drain();
  threadpool.enqueue(add, 3, 4);
  threadpool.enqueue(add, 3, 4);
  threadpool.reset();
  threadpool.enqueue(add, 3, 4);
  threadpool.enqueue(add, 3, 4);
  int pendingTasksCount;
  threadpool.status(pendingTasksCount);
  return 0;
}
