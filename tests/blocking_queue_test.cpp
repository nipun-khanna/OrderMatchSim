#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "common/utils/BlockingQueue.h"

using oms::util::BlockingQueue;

TEST(BlockingQueue, PreservesFifoOrder) {
  BlockingQueue<int> queue;
  queue.push(1);
  queue.push(2);
  queue.push(3);

  int out = 0;
  ASSERT_TRUE(queue.pop(out));
  EXPECT_EQ(out, 1);
  ASSERT_TRUE(queue.pop(out));
  EXPECT_EQ(out, 2);
  ASSERT_TRUE(queue.pop(out));
  EXPECT_EQ(out, 3);
}

TEST(BlockingQueue, PopReturnsFalseWhenClosedAndEmpty) {
  BlockingQueue<int> queue;
  queue.close();

  int out = 0;
  EXPECT_FALSE(queue.pop(out));
}

TEST(BlockingQueue, CloseDrainsRemainingItemsBeforeStopping) {
  BlockingQueue<int> queue;
  queue.push(10);
  queue.push(20);
  queue.close();

  int out = 0;
  ASSERT_TRUE(queue.pop(out));
  EXPECT_EQ(out, 10);
  ASSERT_TRUE(queue.pop(out));
  EXPECT_EQ(out, 20);
  EXPECT_FALSE(queue.pop(out));
}

TEST(BlockingQueue, TransfersAllItemsAcrossThreads) {
  BlockingQueue<int> queue;
  constexpr int kCount = 10000;

  std::atomic<long long> sum{0};
  std::thread consumer([&] {
    int value = 0;
    while (queue.pop(value)) {
      sum.fetch_add(value, std::memory_order_relaxed);
    }
  });

  std::thread producer([&] {
    for (int i = 1; i <= kCount; ++i) {
      queue.push(i);
    }
    queue.close();
  });

  producer.join();
  consumer.join();

  const long long expected = static_cast<long long>(kCount) * (kCount + 1) / 2;
  EXPECT_EQ(sum.load(), expected);
}
