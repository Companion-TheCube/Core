#include <gtest/gtest.h>
#include "../../src/utils.h"
#include <chrono>
#include <thread>

TEST(CountingLatch, WaitImmediateWhenZero)
{
    CountingLatch latch(0);
    auto start = std::chrono::steady_clock::now();
    latch.wait();
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 10);
}

TEST(CountingLatch, CountDownReleasesWait)
{
    CountingLatch latch(2);
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        latch.count_down();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        latch.count_down();
    });

    auto start = std::chrono::steady_clock::now();
    latch.wait();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    t.join();

    EXPECT_GE(ms, 90); // should wait roughly for both sleeps
}

TEST(CountingLatch, CountUpRequiresExtraCountdown)
{
    CountingLatch latch(1);
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_up(2); // now total 3
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_down();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_down();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        latch.count_down();
    });

    latch.wait();
    t.join();
    SUCCEED();
}

