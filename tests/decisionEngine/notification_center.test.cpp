#include <gtest/gtest.h>

#include "../../src/database/cubeDB.h"
#include "../../src/decisionEngine/notificationCenter.h"
#include "../../src/decisionEngine/scheduler.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace DecisionEngine;

namespace {

bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    return predicate();
}

class NotificationCenterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        prevCwd_ = std::filesystem::current_path();
        tempRoot_ = std::filesystem::temp_directory_path() / ("cube_notification_center_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempRoot_);
        std::filesystem::current_path(tempRoot_);

        dbManager_ = std::make_shared<CubeDatabaseManager>();
        CubeDB::setCubeDBManager(dbManager_);
        dbManager_->openAll();
    }

    void TearDown() override
    {
        NotificationCenter::setSharedInstance({});
        CubeDB::setCubeDBManager(nullptr);
        dbManager_.reset();

        std::filesystem::current_path(prevCwd_);
        std::error_code ec;
        std::filesystem::remove_all(tempRoot_, ec);
    }

    std::shared_ptr<NotificationCenter> makeCenter(std::shared_ptr<Scheduler> scheduler, std::atomic<int>* reminderDisplays = nullptr, std::atomic<int>* alarmDisplays = nullptr)
    {
        auto center = std::make_shared<NotificationCenter>();
        center->setScheduler(scheduler);

        NotificationCenter::PresenterCallbacks callbacks;
        callbacks.showNotification = [](const NotificationCenter::Item&) {};
        callbacks.showReminder = [reminderDisplays](const NotificationCenter::Item&) {
            if (reminderDisplays) {
                reminderDisplays->fetch_add(1);
            }
        };
        callbacks.showAlarm = [alarmDisplays](const NotificationCenter::Item&) {
            if (alarmDisplays) {
                alarmDisplays->fetch_add(1);
            }
        };
        callbacks.startAlarmSound = []() {};
        callbacks.stopAlarmSound = []() {};
        callbacks.playReminderSound = []() {};
        center->setPresenterCallbacks(callbacks);
        return center;
    }

    std::filesystem::path prevCwd_;
    std::filesystem::path tempRoot_;
    std::shared_ptr<CubeDatabaseManager> dbManager_;
};

} // namespace

TEST_F(NotificationCenterTest, GenericNotificationIsPersistedAndListed)
{
    auto scheduler = std::make_shared<Scheduler>();
    auto center = makeCenter(scheduler);
    scheduler->start();
    center->start();

    const auto id = center->createNotification("Build complete", "Tests passed", "system", "normal", { { "suite", "core" } }, false);
    ASSERT_GT(id, 0);

    const auto item = center->getItem(id);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->kind, NotificationCenter::Kind::NOTIFICATION);
    EXPECT_EQ(item->title, "Build complete");
    EXPECT_EQ(item->message, "Tests passed");
    EXPECT_EQ(item->source, "system");
    EXPECT_TRUE(item->metadata.contains("suite"));

    const auto recent = center->listRecent(10);
    ASSERT_FALSE(recent.empty());
    EXPECT_EQ(recent.front().id, id);

    center->stop();
    scheduler->stop();
}

TEST_F(NotificationCenterTest, ReminderSurvivesRestartAndDeliversOnce)
{
    std::atomic<int> reminderDisplays { 0 };

    auto scheduler = std::make_shared<Scheduler>();
    auto center = makeCenter(scheduler, &reminderDisplays);
    scheduler->start();
    center->start();

    const auto dueMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count()
        + 350;
    const auto id = center->createReminder("Stretch", "Time to stretch", dueMs);
    ASSERT_GT(id, 0);

    center->stop();
    scheduler->stop();
    center.reset();
    scheduler.reset();

    auto scheduler2 = std::make_shared<Scheduler>();
    auto center2 = makeCenter(scheduler2, &reminderDisplays);
    scheduler2->start();
    center2->start();

    ASSERT_TRUE(waitUntil([&]() {
        const auto item = center2->getItem(id);
        return item.has_value() && item->deliveredAtEpochMs > 0 && !item->active;
    }, std::chrono::seconds(3)));

    EXPECT_EQ(reminderDisplays.load(), 1);

    center2->stop();
    scheduler2->stop();
}

TEST_F(NotificationCenterTest, SnoozedAlarmMovesScheduledTimeForward)
{
    std::atomic<int> alarmDisplays { 0 };

    auto scheduler = std::make_shared<Scheduler>();
    auto center = makeCenter(scheduler, nullptr, &alarmDisplays);
    scheduler->start();
    center->start();

    const auto dueMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count()
        + 150;
    const auto id = center->createAlarm("Wake up", "Wake up", dueMs);
    ASSERT_GT(id, 0);

    ASSERT_TRUE(waitUntil([&]() {
        const auto item = center->getItem(id);
        return item.has_value() && item->deliveredAtEpochMs > 0 && item->active;
    }, std::chrono::seconds(3)));

    const auto deliveredItem = center->getItem(id);
    ASSERT_TRUE(deliveredItem.has_value());
    const auto previousScheduledFor = deliveredItem->scheduledForEpochMs;

    ASSERT_TRUE(center->snooze(id, 60 * 1000));

    const auto snoozedItem = center->getItem(id);
    ASSERT_TRUE(snoozedItem.has_value());
    EXPECT_TRUE(snoozedItem->active);
    EXPECT_FALSE(snoozedItem->acknowledged);
    EXPECT_GT(snoozedItem->scheduledForEpochMs, previousScheduledFor);
    EXPECT_GE(alarmDisplays.load(), 1);

    center->stop();
    scheduler->stop();
}

TEST_F(NotificationCenterTest, AcknowledgingRepeatingAlarmReschedulesIt)
{
    std::atomic<int> alarmDisplays { 0 };

    auto scheduler = std::make_shared<Scheduler>();
    auto center = makeCenter(scheduler, nullptr, &alarmDisplays);
    scheduler->start();
    center->start();

    const auto dueMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count()
        + 150;
    const auto id = center->createAlarm(
        "Daily alarm",
        "Daily alarm",
        dueMs,
        NotificationCenter::RepeatRule::CUSTOM_SECONDS,
        60);
    ASSERT_GT(id, 0);

    ASSERT_TRUE(waitUntil([&]() {
        const auto item = center->getItem(id);
        return item.has_value() && item->deliveredAtEpochMs > 0 && item->active;
    }, std::chrono::seconds(3)));

    ASSERT_TRUE(center->acknowledge(id));
    const auto updated = center->getItem(id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_TRUE(updated->acknowledged);
    EXPECT_TRUE(updated->active);
    EXPECT_GT(updated->scheduledForEpochMs, updated->deliveredAtEpochMs);

    center->stop();
    scheduler->stop();
}

TEST_F(NotificationCenterTest, StopJoinsAlarmPlaybackBeforeDestroy)
{
    auto scheduler = std::make_shared<Scheduler>();
    auto center = makeCenter(scheduler);
    scheduler->start();
    center->start();

    const auto dueMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count()
        + 150;
    const auto id = center->createAlarm("Shutdown alarm", "Shutdown alarm", dueMs);
    ASSERT_GT(id, 0);

    ASSERT_TRUE(waitUntil([&]() {
        const auto item = center->getItem(id);
        return item.has_value() && item->deliveredAtEpochMs > 0 && item->active;
    }, std::chrono::seconds(3)));

    center->stop();
    scheduler->stop();
    center.reset();
    scheduler.reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(2200));
    SUCCEED();
}

TEST_F(NotificationCenterTest, VoiceAlarmParserPreservesMinutesForExplicitToday)
{
    const auto now = std::chrono::system_clock::now();
    const auto target = now + std::chrono::hours(2);
    const auto nowT = std::chrono::system_clock::to_time_t(now);
    const auto targetT = std::chrono::system_clock::to_time_t(target);

    std::tm nowTm {};
    std::tm targetTm {};
#ifdef _WIN32
    localtime_s(&nowTm, &nowT);
    localtime_s(&targetTm, &targetT);
#else
    localtime_r(&nowT, &nowTm);
    localtime_r(&targetT, &targetTm);
#endif

    if (nowTm.tm_yday != targetTm.tm_yday) {
        GTEST_SKIP() << "No stable same-day future slot available for explicit 'today' parsing test.";
    }

    auto scheduler = std::make_shared<Scheduler>();
    auto center = makeCenter(scheduler);
    scheduler->start();
    center->start();

    std::ostringstream utterance;
    utterance << "set an alarm for "
              << std::put_time(&targetTm, "%I:%M%p")
              << " today";

    const auto result = center->createAlarmFromTranscript(utterance.str());
    ASSERT_EQ(result.value("status", ""), "ok");

    const auto id = result.value("notificationId", -1L);
    ASSERT_GT(id, 0);

    const auto item = center->getItem(id);
    ASSERT_TRUE(item.has_value());

    const auto scheduledT = static_cast<std::time_t>(item->scheduledForEpochMs / 1000LL);
    std::tm scheduledTm {};
#ifdef _WIN32
    localtime_s(&scheduledTm, &scheduledT);
#else
    localtime_r(&scheduledT, &scheduledTm);
#endif

    EXPECT_EQ(scheduledTm.tm_year, targetTm.tm_year);
    EXPECT_EQ(scheduledTm.tm_yday, targetTm.tm_yday);
    EXPECT_EQ(scheduledTm.tm_hour, targetTm.tm_hour);
    EXPECT_EQ(scheduledTm.tm_min, targetTm.tm_min);

    center->stop();
    scheduler->stop();
}
