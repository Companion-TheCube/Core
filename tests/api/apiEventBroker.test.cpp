#include "../../src/api/apiEventBroker.h"
#include <gtest/gtest.h>

#include <future>
#include <thread>

namespace {

TEST(ApiEventBrokerTest, ReturnsReplayPagesUsingStableCursorSemantics)
{
    ApiEventBroker broker(8);
    broker.registerSource("interaction");

    broker.publish("interaction", "tap", nlohmann::json::object(), 1000);
    broker.publish("interaction", "lift_started", nlohmann::json::object(), 1200);
    broker.publish("interaction", "lift_ended", nlohmann::json::object(), 1400);

    const auto firstPage = broker.waitForEvents(
        0,
        ApiEventBroker::SourceSet { "interaction" },
        2,
        std::chrono::milliseconds::zero());

    ASSERT_EQ(firstPage.events.size(), 2u);
    EXPECT_EQ(firstPage.events[0].sequence, 1u);
    EXPECT_EQ(firstPage.events[1].sequence, 2u);
    EXPECT_EQ(firstPage.nextSequence, 2u);
    EXPECT_FALSE(firstPage.historyTruncated);
    EXPECT_FALSE(firstPage.timedOut);

    const auto secondPage = broker.waitForEvents(
        firstPage.nextSequence,
        ApiEventBroker::SourceSet { "interaction" },
        2,
        std::chrono::milliseconds::zero());

    ASSERT_EQ(secondPage.events.size(), 1u);
    EXPECT_EQ(secondPage.events[0].sequence, 3u);
    EXPECT_EQ(secondPage.nextSequence, 3u);
}

TEST(ApiEventBrokerTest, ReportsHistoryTruncationWhenCallerFallsBehind)
{
    ApiEventBroker broker(2);
    broker.registerSource("interaction");

    broker.publish("interaction", "tap", nlohmann::json::object(), 1000);
    broker.publish("interaction", "lift_started", nlohmann::json::object(), 1200);
    broker.publish("interaction", "lift_ended", nlohmann::json::object(), 1400);

    const auto page = broker.waitForEvents(
        0,
        ApiEventBroker::SourceSet { "interaction" },
        10,
        std::chrono::milliseconds::zero());

    EXPECT_TRUE(page.historyTruncated);
    ASSERT_EQ(page.events.size(), 2u);
    EXPECT_EQ(page.events.front().sequence, 2u);
    EXPECT_EQ(page.events.back().sequence, 3u);
    EXPECT_EQ(page.nextSequence, 3u);
}

TEST(ApiEventBrokerTest, WaitsForMatchingEventsAndTimesOutWhenNoneArrive)
{
    ApiEventBroker broker(8);
    broker.registerSource("interaction");

    auto future = std::async(std::launch::async, [&broker]() {
        return broker.waitForEvents(
            0,
            ApiEventBroker::SourceSet { "interaction" },
            10,
            std::chrono::milliseconds(200));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    broker.publish("interaction", "tap", nlohmann::json::object(), 1000);

    const auto page = future.get();
    ASSERT_EQ(page.events.size(), 1u);
    EXPECT_EQ(page.events[0].event, "tap");
    EXPECT_FALSE(page.timedOut);

    const auto timeoutPage = broker.waitForEvents(
        page.nextSequence,
        ApiEventBroker::SourceSet { "interaction" },
        10,
        std::chrono::milliseconds(10));

    EXPECT_TRUE(timeoutPage.events.empty());
    EXPECT_TRUE(timeoutPage.timedOut);
    EXPECT_EQ(timeoutPage.nextSequence, page.nextSequence);
}

} // namespace
