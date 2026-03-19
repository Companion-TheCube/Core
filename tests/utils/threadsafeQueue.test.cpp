#include <gtest/gtest.h>

#include "../../src/threadsafeQueue.h"

#include <chrono>
#include <future>
#include <optional>
#include <thread>

using namespace std::chrono_literals;

TEST(ThreadSafeQueue, CloseUnblocksBlockedPop)
{
    ThreadSafeQueue<int> queue;
    std::promise<std::optional<int>> resultPromise;
    auto resultFuture = resultPromise.get_future();

    std::thread consumer([&]() {
        resultPromise.set_value(queue.pop());
    });

    queue.close();

    ASSERT_EQ(resultFuture.wait_for(250ms), std::future_status::ready);
    EXPECT_FALSE(resultFuture.get().has_value());

    consumer.join();
}

TEST(ThreadSafeQueue, CloseWithClearDropsPendingItems)
{
    ThreadSafeQueue<int> queue;
    queue.push(42);

    queue.close(true);

    auto value = queue.pop();
    EXPECT_FALSE(value.has_value());
}

TEST(ThreadSafeQueue, ReopenAllowsNewTrafficAfterClose)
{
    ThreadSafeQueue<int> queue;
    queue.close(true);
    queue.reopen(true);

    queue.push(7);
    auto value = queue.pop();

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 7);
}
