#include "../src/threadpool.h"

#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>

namespace {
class ThreadpoolTest : public testing::Test {
  void SetUp() override {
    free.store(false);
    tasksExecuted.store(0);
    threadpool =
        std::make_unique<Threadpool::Threadpool>(NUMBER_OF_WORKERS, QUEUE_SIZE);
  }

  void TearDown() override { release(); }

protected:
  int NUMBER_OF_WORKERS = 2, QUEUE_SIZE = 2;
  std::atomic<bool> free;
  std::atomic<int> tasksExecuted;
  std::unique_ptr<Threadpool::Threadpool> threadpool;
  std::condition_variable cv;
  std::mutex mutexProcess;
  std::function<void()> test_func = [&]() {
    std::cout << "Test Function" << std::endl;
    std::unique_lock lock(mutexProcess);
    cv.wait(lock, [&] { return free.load(); });
    ++tasksExecuted;
    std::cout << "Test Function Complete." << std::endl;
  };

  void release() {
    free.store(true);
    cv.notify_all();
  }

  void pendingTasksCount() {
    for (auto i = 0; i < NUMBER_OF_WORKERS; ++i) {
      threadpool->enqueue(test_func);
    }
    threadpool->drain();
    threadpool->reset();
  }

  void fillQueue() {
    for (auto i = 0; i < NUMBER_OF_WORKERS; ++i) {
      Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(test_func);
      ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_SUCCESS);
    }
    threadpool->drain();
    threadpool->reset();
    for (auto i = 0; i < QUEUE_SIZE; ++i) {
      Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(test_func);
      ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_SUCCESS);
    }
  }
};

TEST_F(ThreadpoolTest, EnqueuedTaskIsExecuted) {
  // Given
  std::atomic<bool> executed{false};
  std::mutex mutexExecuted;
  std::condition_variable cvExecuted;
  auto temptask = [&] {
    executed.store(true);
    cvExecuted.notify_one();
  };

  // When
  std::unique_lock lockExecuted(mutexExecuted);
  Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(temptask);
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_SUCCESS);
  cvExecuted.wait(lockExecuted, [&] { return executed.load(); });

  // Then
  ASSERT_TRUE(executed.load());
}

TEST_F(ThreadpoolTest, AllTasksAreExecuted) {
  // Given
  fillQueue();
  ASSERT_EQ(tasksExecuted, 0);

  // When
  release();
  threadpool->shutdown();

  // Then
  ASSERT_EQ(tasksExecuted, NUMBER_OF_WORKERS + QUEUE_SIZE);
}

TEST_F(ThreadpoolTest, DrainWaitsForTasksToFinish) {
  // Given
  fillQueue();
  int count;
  threadpool->status(count);
  ASSERT_EQ(count, QUEUE_SIZE);

  // When
  release();
  threadpool->drain();
  threadpool->status(count);

  // Then
  ASSERT_EQ(count, 0);
}

TEST_F(ThreadpoolTest, ConcurrentEnqueueIsSafe) {
  constexpr int THREADS = 4;
  constexpr int TASKS_PER_THREAD = 10;

  std::atomic<int> executed{0}, failed{0};

  std::vector<std::thread> producers;

  for (int i = 0; i < THREADS; ++i) {
    producers.emplace_back([&] {
      for (int j = 0; j < TASKS_PER_THREAD; ++j) {
        auto rc = threadpool->enqueue([&] { ++executed; });

        // Queue may legitimately become full.
        if (rc != Threadpool::ThreadpoolStatusCode::e_SUCCESS) {
          ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_QUEUE_FULL);
          ++failed;
        }
      }
    });
  }

  for (auto &producer : producers) {
    producer.join();
  }

  threadpool->drain();
  ASSERT_EQ(executed + failed, THREADS * TASKS_PER_THREAD);
}

TEST_F(ThreadpoolTest, EnqueueSuccess) {
  // When
  Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(test_func);

  // Then
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_SUCCESS);
}

TEST_F(ThreadpoolTest, EnqueueFull) {
  // Given
  fillQueue();

  // When
  Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(test_func);

  // Then
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_QUEUE_FULL);
}

TEST_F(ThreadpoolTest, EnqueueDrained) {
  // Given
  threadpool->drain();

  // When
  Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(test_func);

  // Then
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_DRAINED);
}

TEST_F(ThreadpoolTest, EnqueueShutdown) {
  // Given
  threadpool->shutdown();

  // When
  Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(test_func);

  // Then
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_DEAD);
}

TEST_F(ThreadpoolTest, EnqueueFetchesNewTasksFromQueue) {
  // Given
  fillQueue();
  int count;
  Threadpool::ThreadpoolStatusCode rc = threadpool->status(count);
  ASSERT_EQ(count, QUEUE_SIZE);
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_ACTIVE);

  // When
  release();
  threadpool->drain();
  threadpool->reset();
  rc = threadpool->status(count);

  // Then
  ASSERT_EQ(count, 0);
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_ACTIVE);
}

TEST_F(ThreadpoolTest, ResetSuccess) {
  // Given
  threadpool->drain();
  Threadpool::ThreadpoolStatusCode rc = threadpool->enqueue(test_func);
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_DRAINED);
  int count;

  // When
  threadpool->reset();
  rc = threadpool->enqueue(test_func);

  // Then
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_SUCCESS);
  rc = threadpool->status(count);
  ASSERT_EQ(count, 1);
}

TEST_F(ThreadpoolTest, StatusDrained) {
  // Given
  threadpool->drain();
  int count;

  // When
  Threadpool::ThreadpoolStatusCode rc = threadpool->status(count);

  // Then
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_DRAINED);
}

TEST_F(ThreadpoolTest, StatusDead) {
  // Given
  threadpool->shutdown();
  int count;

  // When
  Threadpool::ThreadpoolStatusCode rc = threadpool->status(count);

  // Then
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_DEAD);
}

TEST_F(ThreadpoolTest, StatusCount) {
  // Given
  int count;
  Threadpool::ThreadpoolStatusCode rc = threadpool->status(count);
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_ACTIVE);
  ASSERT_EQ(count, 0);

  pendingTasksCount();

  // When
  threadpool->enqueue(test_func);

  // Then
  rc = threadpool->status(count);
  ASSERT_EQ(rc, Threadpool::ThreadpoolStatusCode::e_ACTIVE);
  ASSERT_EQ(count, 1);
}

} // namespace
