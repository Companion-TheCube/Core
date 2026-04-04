/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#pragma once

#include "../api/autoRegister.h"
#include "../database/cubeDB.h"
#include "scheduler.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace DecisionEngine {

class NotificationCenter : public AutoRegisterAPI<NotificationCenter> {
public:
    enum class Kind {
        NOTIFICATION,
        REMINDER,
        ALARM
    };

    enum class RepeatRule {
        NONE,
        DAILY,
        WEEKLY,
        CUSTOM_SECONDS
    };

    struct Item {
        long id = -1;
        Kind kind = Kind::NOTIFICATION;
        std::string title;
        std::string message;
        int64_t timeLegacyEpochMs = 0;
        int64_t createdAtEpochMs = 0;
        int64_t scheduledForEpochMs = 0;
        int64_t deliveredAtEpochMs = 0;
        std::string source = "core";
        bool read = false;
        bool acknowledged = false;
        bool active = false;
        std::string priority = "normal";
        RepeatRule repeatRule = RepeatRule::NONE;
        int64_t repeatIntervalSeconds = 0;
        nlohmann::json metadata = nlohmann::json::object();

        nlohmann::json toJson() const;
    };

    struct PresenterCallbacks {
        std::function<void(const Item&)> showNotification;
        std::function<void(const Item&)> showReminder;
        std::function<void(const Item&)> showAlarm;
        std::function<void()> startAlarmSound;
        std::function<void()> stopAlarmSound;
        std::function<void()> playReminderSound;
    };

    NotificationCenter();
    ~NotificationCenter();

    void setScheduler(const std::shared_ptr<Scheduler>& scheduler);
    void setPresenterCallbacks(PresenterCallbacks callbacks);

    void start();
    void stop();

    long createNotification(
        const std::string& title,
        const std::string& message,
        const std::string& source = "core",
        const std::string& priority = "normal",
        const nlohmann::json& metadata = nlohmann::json::object(),
        bool displayNow = true);

    long createReminder(
        const std::string& title,
        const std::string& message,
        int64_t scheduledForEpochMs,
        RepeatRule repeatRule = RepeatRule::NONE,
        int64_t repeatIntervalSeconds = 0,
        const std::string& source = "core",
        const nlohmann::json& metadata = nlohmann::json::object());

    long createAlarm(
        const std::string& title,
        const std::string& message,
        int64_t scheduledForEpochMs,
        RepeatRule repeatRule = RepeatRule::NONE,
        int64_t repeatIntervalSeconds = 0,
        const std::string& source = "core",
        const nlohmann::json& metadata = nlohmann::json::object());

    std::vector<Item> listRecent(size_t limit = 25) const;
    std::vector<Item> listUpcomingReminders(size_t limit = 10) const;
    std::vector<Item> listActiveAlarms(size_t limit = 10) const;
    std::optional<Item> getItem(long id) const;

    bool acknowledge(long id);
    bool dismiss(long id);
    bool snooze(long id, int64_t delayMs);
    bool markRead(long id, bool read);

    std::string recentSummary(size_t limit = 10) const;

    nlohmann::json createReminderFromArgs(const nlohmann::json& args);
    nlohmann::json createAlarmFromArgs(const nlohmann::json& args);
    nlohmann::json createReminderFromTranscript(const std::string& transcript);
    nlohmann::json createAlarmFromTranscript(const std::string& transcript);
    nlohmann::json listUpcomingRemindersResult() const;
    nlohmann::json listActiveAlarmsResult() const;
    nlohmann::json cancelFromArgs(const nlohmann::json& args);
    nlohmann::json snoozeFromArgs(const nlohmann::json& args);
    nlohmann::json cancelFromTranscript(const std::string& transcript);
    nlohmann::json snoozeFromTranscript(const std::string& transcript);

    static void setSharedInstance(const std::shared_ptr<NotificationCenter>& instance);
    static std::shared_ptr<NotificationCenter> sharedInstance();

    std::string getInterfaceName() const override { return "Notifications"; }
    HttpEndPointData_t getHttpEndpointData() override;

private:
    mutable std::mutex mutex_;
    std::shared_ptr<Scheduler> scheduler_;
    std::unordered_map<long, uint32_t> scheduledHandles_;
    PresenterCallbacks presenterCallbacks_;
    std::atomic<uint64_t> alarmPlaybackGeneration_ { 0 };
    std::jthread alarmPlaybackThread_;
    long activeAlarmId_ = -1;
    bool started_ = false;

    void ensureSchema() const;
    void reloadScheduledItems();
    void scheduleItem(const Item& item);
    void unscheduleItem(long id);
    void deliverItem(long id);
    std::jthread takeAlarmPlaybackThreadLocked();
    void startAlarmLoop(long id);
    void stopAlarmLoop();
    void presentItem(const Item& item);
    bool updateItem(const Item& item);
    int64_t computeNextScheduledFor(const Item& item, int64_t nowEpochMs) const;
};

} // namespace DecisionEngine
