// Purpose: Exercise TaskQueue<T> thread-safe behavior and ordering semantics.
//
// The implementation provides three core operations:
// - push(T): appends to the back and wakes one waiter.
// - pop(): blocks until non-empty, then removes from the back (LIFO behavior).
// - shift(): blocks until non-empty, then removes from the front (FIFO behavior).
// Additional helpers: peek()/peek(index) and size().
#include <gtest/gtest.h>
#include "../../src/utils.h"
#include <atomic>
#include <thread>
#include <vector>

TEST(TaskQueue, PushPop_LIFO)
{
    // We push in ascending order, but use pop(), which removes from the back.
    // Expectation: the consumer observes values in reverse order (LIFO).
    TaskQueue<int> q;
    std::vector<int> pushed{1, 2, 3};

    // Producer thread pushes all elements. push() notifies one waiter, if any.
    std::thread producer([&]() {
        for (int v : pushed) {
            q.push(v);
        }
    });

    // Consumer thread calls pop() exactly pushed.size() times and collects results.
    std::vector<int> popped;
    std::thread consumer([&]() {
        for (size_t i = 0; i < pushed.size(); ++i) {
            popped.push_back(q.pop());
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(popped.size(), pushed.size());
    // Because pop() removes from the back, order is reversed relative to insertion.
    std::vector<int> expected{3, 2, 1};
    EXPECT_EQ(popped, expected);
}

TEST(TaskQueue, Shift_FIFO)
{
    // Using shift() removes from the front, so the observed order matches insertion.
    TaskQueue<int> q;
    q.push(10);
    q.push(20);
    q.push(30);

    // Validate FIFO behavior.
    EXPECT_EQ(q.shift(), 10);
    EXPECT_EQ(q.shift(), 20);
    EXPECT_EQ(q.shift(), 30);
}

TEST(TaskQueue, SizeAndPeek)
{
    // size() and peek() accessors should reflect the current queue state without mutation.
    TaskQueue<int> q;
    q.push(7);
    q.push(8);

    // size() reports the number of elements present.
    EXPECT_EQ(q.size(), 2u);
    // peek() returns the current front element without removing it.
    EXPECT_EQ(q.peek(), 7);
    // peek(index) returns an element at the given position; here index 1 -> second element.
    EXPECT_EQ(q.peek(1), 8);
}
