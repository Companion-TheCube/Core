#include <gtest/gtest.h>
#include "../../src/utils.h"
#include <atomic>
#include <thread>
#include <vector>

TEST(TaskQueue, PushPop_LIFO)
{
    TaskQueue<int> q;
    std::vector<int> pushed{1, 2, 3};

    std::thread producer([&]() {
        for (int v : pushed) {
            q.push(v);
        }
    });

    std::vector<int> popped;
    std::thread consumer([&]() {
        for (size_t i = 0; i < pushed.size(); ++i) {
            popped.push_back(q.pop());
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(popped.size(), pushed.size());
    // pop() takes from back, so order should be reverse of pushed
    std::vector<int> expected{3, 2, 1};
    EXPECT_EQ(popped, expected);
}

TEST(TaskQueue, Shift_FIFO)
{
    TaskQueue<int> q;
    q.push(10);
    q.push(20);
    q.push(30);

    // shift() takes from front, so FIFO
    EXPECT_EQ(q.shift(), 10);
    EXPECT_EQ(q.shift(), 20);
    EXPECT_EQ(q.shift(), 30);
}

TEST(TaskQueue, SizeAndPeek)
{
    TaskQueue<int> q;
    q.push(7);
    q.push(8);
    EXPECT_EQ(q.size(), 2u);
    EXPECT_EQ(q.peek(), 7);
    EXPECT_EQ(q.peek(1), 8);
}

