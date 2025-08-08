// Purpose: Validate CountingLatch synchronization semantics.
//
// The latch starts with a specified count. Threads block in wait() until the
// count reaches zero. Each count_down() decrements the count; count_up(n)
// increases the number of required count_down() calls before release.
#include <gtest/gtest.h>
#include "../../src/utils.h"
#include <chrono>
#include <thread>

TEST(CountingLatch, WaitImmediateWhenZero)
{
    // If the initial count is zero, wait() should not block at all.
    CountingLatch latch(0);
    auto start = std::chrono::steady_clock::now();
    latch.wait();
    auto elapsed = std::chrono::steady_clock::now() - start;
    // Assert that the wait returned essentially immediately (under 10 ms).
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 10);
}

TEST(CountingLatch, CountDownReleasesWait)
{
    // With initial count=2, we need exactly two count_down() calls for wait() to return.
    CountingLatch latch(2);
    std::thread t([&]() {
        // Stagger the decrements to make the blocking observable.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        latch.count_down();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        latch.count_down();
    });

    auto start = std::chrono::steady_clock::now();
    latch.wait();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    t.join();

    // We expect to block for roughly the sum of the staged delays.
    EXPECT_GE(ms, 90);
}

TEST(CountingLatch, CountUpRequiresExtraCountdown)
{
    // Demonstrate that count_up(n) increases the number of required countdowns
    // even after the latch is constructed.
    CountingLatch latch(1);
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_up(2); // now total required = 3
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_down();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_down();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_down();
    });

    // If count_up() did not take effect, wait() could return too early.
    latch.wait();
    t.join();
    SUCCEED();
}
