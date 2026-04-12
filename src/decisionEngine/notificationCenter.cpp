/*
███╗   ██╗ ██████╗ ████████╗██╗███████╗██╗ ██████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗ ██████╗███████╗███╗   ██╗████████╗███████╗██████╗     ██████╗██████╗ ██████╗
████╗  ██║██╔═══██╗╚══██╔══╝██║██╔════╝██║██╔════╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝██╔════╝████╗  ██║╚══██╔══╝██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██╔██╗ ██║██║   ██║   ██║   ██║█████╗  ██║██║     ███████║   ██║   ██║██║   ██║██╔██╗ ██║██║     █████╗  ██╔██╗ ██║   ██║   █████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██║╚██╗██║██║   ██║   ██║   ██║██╔══╝  ██║██║     ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║██║     ██╔══╝  ██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
██║ ╚████║╚██████╔╝   ██║   ██║██║     ██║╚██████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║╚██████╗███████╗██║ ╚████║   ██║   ███████╗██║  ██║██╗╚██████╗██║     ██║
╚═╝  ╚═══╝ ╚═════╝    ╚═╝   ╚═╝╚═╝     ╚═╝ ╚═════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License
Copyright (c) 2026 A-McD Technology LLC
*/

#include "notificationCenter.h"

#include "../audio/audioOutput.h"
#include "../gui/gui.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <future>
#include <iomanip>
#include <numeric>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <thread>

using namespace DecisionEngine;

namespace {

std::weak_ptr<NotificationCenter> gSharedNotificationCenter;

int64_t nowEpochMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string lowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string trim(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string kindToString(NotificationCenter::Kind kind)
{
    switch (kind) {
    case NotificationCenter::Kind::NOTIFICATION:
        return "notification";
    case NotificationCenter::Kind::REMINDER:
        return "reminder";
    case NotificationCenter::Kind::ALARM:
        return "alarm";
    }
    return "notification";
}

NotificationCenter::Kind kindFromString(const std::string& value)
{
    const auto lower = lowerAscii(value);
    if (lower == "reminder") return NotificationCenter::Kind::REMINDER;
    if (lower == "alarm") return NotificationCenter::Kind::ALARM;
    return NotificationCenter::Kind::NOTIFICATION;
}

std::string repeatRuleToString(NotificationCenter::RepeatRule repeatRule, int64_t repeatIntervalSeconds)
{
    switch (repeatRule) {
    case NotificationCenter::RepeatRule::NONE:
        return "none";
    case NotificationCenter::RepeatRule::DAILY:
        return "daily";
    case NotificationCenter::RepeatRule::WEEKLY:
        return "weekly";
    case NotificationCenter::RepeatRule::CUSTOM_SECONDS:
        return "custom:" + std::to_string(repeatIntervalSeconds);
    }
    return "none";
}

NotificationCenter::RepeatRule repeatRuleFromString(const std::string& value, int64_t& repeatIntervalSeconds)
{
    const auto lower = lowerAscii(value);
    if (lower == "daily") return NotificationCenter::RepeatRule::DAILY;
    if (lower == "weekly") return NotificationCenter::RepeatRule::WEEKLY;
    if (lower.rfind("custom:", 0) == 0) {
        try {
            repeatIntervalSeconds = std::stoll(lower.substr(7));
        } catch (...) {
            repeatIntervalSeconds = 0;
        }
        return NotificationCenter::RepeatRule::CUSTOM_SECONDS;
    }
    repeatIntervalSeconds = 0;
    return NotificationCenter::RepeatRule::NONE;
}

int64_t parseEpoch(const std::string& value)
{
    if (value.empty()) return 0;
    try {
        return std::stoll(value);
    } catch (...) {
        return 0;
    }
}

std::optional<int64_t> parseLocalIsoDateTimeMs(const std::string& rawValue)
{
    auto value = trim(rawValue);
    if (value.empty()) {
        return std::nullopt;
    }

    for (char& ch : value) {
        if (ch == ' ') {
            ch = 'T';
        }
    }

    static const std::array<const char*, 3> formats = {
        "%Y-%m-%dT%H:%M:%S",
        "%Y-%m-%dT%H:%M",
        "%Y-%m-%dT%H"
    };

    for (const char* format : formats) {
        std::tm localTm {};
        localTm.tm_isdst = -1;
        std::istringstream iss(value);
        iss >> std::get_time(&localTm, format);
        if (!iss.fail() && iss.rdbuf()->in_avail() == 0) {
            const auto localTime = std::mktime(&localTm);
            if (localTime >= 0) {
                return static_cast<int64_t>(localTime) * 1000LL;
            }
        }
    }

    return std::nullopt;
}

std::optional<int64_t> scheduledForEpochMsFromArgs(const nlohmann::json& args)
{
    if (!args.is_object()) {
        return std::nullopt;
    }
    if (args.contains("scheduledForLocalIso") && args["scheduledForLocalIso"].is_string()) {
        if (const auto parsed = parseLocalIsoDateTimeMs(args["scheduledForLocalIso"].get<std::string>())) {
            return parsed;
        }
    }
    if (args.contains("scheduledForEpochMs") && args["scheduledForEpochMs"].is_number_integer()) {
        return args["scheduledForEpochMs"].get<int64_t>();
    }
    return std::nullopt;
}

std::string epochString(int64_t value)
{
    return value > 0 ? std::to_string(value) : "";
}

std::string formatEpochMs(int64_t epochMs)
{
    if (epochMs <= 0) return "unscheduled";
    const auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(epochMs));
    const auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm {};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%A, %B %d at %I:%M %p");
    return out.str();
}

std::filesystem::path defaultAlertSoundPath()
{
    return std::filesystem::path("data") / "wake_sounds" / "song (3).wav";
}

template <typename F>
auto runNotificationDbTask(F&& fn)
{
    using ReturnT = decltype(fn(static_cast<Database*>(nullptr)));
    auto manager = CubeDB::getDBManager();
    if (!manager) {
        throw std::runtime_error("CubeDB manager is not initialized");
    }
    auto promise = std::make_shared<std::promise<ReturnT>>();
    auto future = promise->get_future();
    manager->addDbTask([promise, fn = std::forward<F>(fn), manager]() mutable {
        try {
            auto* db = manager->getDatabase("notifications");
            promise->set_value(fn(db));
        } catch (...) {
            try {
                promise->set_exception(std::current_exception());
            } catch (...) {
            }
        }
    });
    return future.get();
}

std::optional<NotificationCenter::Item> rowToItem(const std::vector<std::string>& row)
{
    if (row.size() < 15) {
        return std::nullopt;
    }
    NotificationCenter::Item item;
    item.id = parseEpoch(row[0]);
    item.kind = kindFromString(row[1]);
    item.title = row[2];
    item.message = row[3];
    item.timeLegacyEpochMs = parseEpoch(row[4]);
    item.createdAtEpochMs = parseEpoch(row[5]);
    item.scheduledForEpochMs = parseEpoch(row[6]);
    item.deliveredAtEpochMs = parseEpoch(row[7]);
    item.source = row[8];
    item.read = row[9] == "1";
    item.acknowledged = row[10] == "1";
    item.active = row[11] == "1";
    item.priority = row[12];
    item.repeatIntervalSeconds = 0;
    item.repeatRule = repeatRuleFromString(row[13], item.repeatIntervalSeconds);
    if (!row[14].empty()) {
        try {
            item.metadata = nlohmann::json::parse(row[14]);
        } catch (...) {
            item.metadata = nlohmann::json::object();
        }
    }
    return item;
}

std::vector<std::string> notificationColumns()
{
    return {
        "id",
        "kind",
        "title",
        "message",
        "time",
        "created_at",
        "scheduled_for",
        "delivered_at",
        "source",
        "read",
        "acknowledged",
        "active",
        "priority",
        "repeat_rule",
        "metadata"
    };
}

struct ScheduledSpec {
    int64_t scheduledForEpochMs = 0;
    NotificationCenter::RepeatRule repeatRule = NotificationCenter::RepeatRule::NONE;
    int64_t repeatIntervalSeconds = 0;
    std::string title;
    std::string message;
};

struct ParsedClockTime {
    int hour = 0;
    int minute = 0;
    bool matched = false;
};

int64_t recentSortKey(const NotificationCenter::Item& item)
{
    if (item.deliveredAtEpochMs > 0) {
        return item.deliveredAtEpochMs;
    }
    if (item.createdAtEpochMs > 0) {
        return item.createdAtEpochMs;
    }
    return item.timeLegacyEpochMs;
}

int64_t activeAlarmSortKey(const NotificationCenter::Item& item)
{
    if (item.deliveredAtEpochMs > 0) {
        return item.deliveredAtEpochMs;
    }
    if (item.scheduledForEpochMs > 0) {
        return item.scheduledForEpochMs;
    }
    return item.createdAtEpochMs;
}

int64_t parseRelativeDelayMs(const std::string& text)
{
    static const std::regex inPattern(R"(in\s+([a-z0-9:\s]+))", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(text, match, inPattern)) {
        return 0;
    }
    const auto segment = lowerAscii(match[1].str());
    static const std::regex partPattern(R"((\d+)\s*(hours?|hrs?|hr|h|minutes?|mins?|min|m|seconds?|secs?|sec|s))");
    auto begin = std::sregex_iterator(segment.begin(), segment.end(), partPattern);
    auto end = std::sregex_iterator();
    int64_t totalMs = 0;
    for (auto it = begin; it != end; ++it) {
        const auto amount = std::stoll((*it)[1].str());
        const auto unit = lowerAscii((*it)[2].str());
        if (unit.starts_with("h")) totalMs += amount * 60LL * 60LL * 1000LL;
        else if (unit.starts_with("m")) totalMs += amount * 60LL * 1000LL;
        else totalMs += amount * 1000LL;
    }
    return totalMs;
}

int64_t parseAbsoluteTimeMs(const std::string& text)
{
    const auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm {};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    const auto lower = lowerAscii(text);
    const bool explicitTomorrow = lower.find("tomorrow") != std::string::npos;
    const bool explicitToday = lower.find("today") != std::string::npos;

    auto tryMatch = [&](const std::regex& pattern, const std::function<ParsedClockTime(const std::smatch&)>& toClockTime) -> ParsedClockTime {
        std::smatch match;
        if (!std::regex_search(text, match, pattern)) {
            return {};
        }
        return toClockTime(match);
    };

    const auto parsed = [&]() -> ParsedClockTime {
        auto parsedTime = tryMatch(
            std::regex(R"(\b(\d{1,2}):(\d{2})\s*(am|pm)\b)", std::regex::icase),
            [](const std::smatch& match) {
                int hour = std::stoi(match[1].str());
                const int minute = std::stoi(match[2].str());
                const auto suffix = lowerAscii(match[3].str());
                if (suffix == "pm" && hour < 12) hour += 12;
                if (suffix == "am" && hour == 12) hour = 0;
                return ParsedClockTime { hour, minute, true };
            });
        if (parsedTime.matched) return parsedTime;

        parsedTime = tryMatch(
            std::regex(R"(\b(\d{1,2}):(\d{2})\b)", std::regex::icase),
            [](const std::smatch& match) {
                return ParsedClockTime { std::stoi(match[1].str()), std::stoi(match[2].str()), true };
            });
        if (parsedTime.matched) return parsedTime;

        parsedTime = tryMatch(
            std::regex(R"(\b(\d{1,2})\s*(am|pm)\b)", std::regex::icase),
            [](const std::smatch& match) {
                int hour = std::stoi(match[1].str());
                const auto suffix = lowerAscii(match[2].str());
                if (suffix == "pm" && hour < 12) hour += 12;
                if (suffix == "am" && hour == 12) hour = 0;
                return ParsedClockTime { hour, 0, true };
            });
        if (parsedTime.matched) return parsedTime;

        return tryMatch(
            std::regex(R"(\b(?:at|for)\s+(\d{1,2})\b)", std::regex::icase),
            [](const std::smatch& match) {
                return ParsedClockTime { std::stoi(match[1].str()), 0, true };
            });
    }();

    if (!parsed.matched || parsed.hour < 0 || parsed.hour > 23 || parsed.minute < 0 || parsed.minute > 59) {
        return 0;
    }

    tm.tm_hour = parsed.hour;
    tm.tm_min = parsed.minute;
    tm.tm_sec = 0;
    auto candidate = std::mktime(&tm);
    if (candidate == -1) return 0;

    if (explicitTomorrow) {
        tm.tm_mday += 1;
        candidate = std::mktime(&tm);
    } else if (!explicitToday && candidate <= tt) {
        tm.tm_mday += 1;
        candidate = std::mktime(&tm);
    } else if (explicitToday && candidate <= tt) {
        return 0;
    }
    return static_cast<int64_t>(candidate) * 1000LL;
}

ScheduledSpec parseReminderTranscript(const std::string& transcript)
{
    ScheduledSpec spec;
    const auto normalized = lowerAscii(trim(transcript));
    spec.repeatRule = normalized.find("every day") != std::string::npos || normalized.find("daily") != std::string::npos
        ? NotificationCenter::RepeatRule::DAILY
        : (normalized.find("every week") != std::string::npos || normalized.find("weekly") != std::string::npos
                  ? NotificationCenter::RepeatRule::WEEKLY
                  : NotificationCenter::RepeatRule::NONE);

    std::smatch match;
    if (std::regex_match(transcript, match, std::regex(R"(^\s*remind me in (.+?) to (.+)\s*$)", std::regex::icase))) {
        spec.title = trim(match[2].str());
        spec.message = spec.title;
        spec.scheduledForEpochMs = nowEpochMs() + parseRelativeDelayMs("in " + match[1].str());
        return spec;
    }
    if (std::regex_match(transcript, match, std::regex(R"(^\s*remind me to (.+?) in (.+)\s*$)", std::regex::icase))) {
        spec.title = trim(match[1].str());
        spec.message = spec.title;
        spec.scheduledForEpochMs = nowEpochMs() + parseRelativeDelayMs("in " + match[2].str());
        return spec;
    }
    if (std::regex_match(transcript, match, std::regex(R"(^\s*remind me to (.+?) (?:at|tomorrow) (.+)\s*$)", std::regex::icase))) {
        spec.title = trim(match[1].str());
        spec.message = spec.title;
        spec.scheduledForEpochMs = parseAbsoluteTimeMs(transcript);
        return spec;
    }

    spec.title = trim(transcript);
    spec.message = spec.title;
    const auto relative = parseRelativeDelayMs(normalized);
    if (relative > 0) {
        spec.scheduledForEpochMs = nowEpochMs() + relative;
    } else {
        spec.scheduledForEpochMs = parseAbsoluteTimeMs(normalized);
    }
    return spec;
}

ScheduledSpec parseAlarmTranscript(const std::string& transcript)
{
    ScheduledSpec spec;
    const auto normalized = lowerAscii(trim(transcript));
    spec.title = "Alarm";
    spec.message = "Alarm";
    spec.repeatRule = normalized.find("every day") != std::string::npos || normalized.find("daily") != std::string::npos
        ? NotificationCenter::RepeatRule::DAILY
        : (normalized.find("every week") != std::string::npos || normalized.find("weekly") != std::string::npos
                  ? NotificationCenter::RepeatRule::WEEKLY
                  : NotificationCenter::RepeatRule::NONE);

    const auto relative = parseRelativeDelayMs(normalized);
    if (relative > 0) {
        spec.scheduledForEpochMs = nowEpochMs() + relative;
    } else {
        spec.scheduledForEpochMs = parseAbsoluteTimeMs(normalized);
    }
    return spec;
}

int64_t parseSnoozeDelayMs(const std::string& transcript)
{
    const auto lower = lowerAscii(transcript);
    const auto delayMs = parseRelativeDelayMs(lower);
    if (delayMs > 0) {
        return delayMs;
    }
    return 10LL * 60LL * 1000LL;
}

std::string joinSummaries(const std::vector<std::string>& summaries)
{
    if (summaries.empty()) {
        return "";
    }
    return std::accumulate(
        std::next(summaries.begin()),
        summaries.end(),
        summaries.front(),
        [](std::string acc, const std::string& value) {
            return std::move(acc) + ", " + value;
        });
}

} // namespace

nlohmann::json NotificationCenter::Item::toJson() const
{
    return nlohmann::json({
        { "id", id },
        { "kind", kindToString(kind) },
        { "title", title },
        { "message", message },
        { "timeEpochMs", timeLegacyEpochMs },
        { "createdAtEpochMs", createdAtEpochMs },
        { "scheduledForEpochMs", scheduledForEpochMs },
        { "deliveredAtEpochMs", deliveredAtEpochMs },
        { "source", source },
        { "read", read },
        { "acknowledged", acknowledged },
        { "active", active },
        { "priority", priority },
        { "repeatRule", repeatRuleToString(repeatRule, repeatIntervalSeconds) },
        { "metadata", metadata }
    });
}

NotificationCenter::NotificationCenter()
{
    presenterCallbacks_.showNotification = [](const Item& item) {
        GUI::showNotification(item.title, item.message, NotificationsManager::NotificationType::NOTIFICATION_OKAY);
    };
    presenterCallbacks_.showReminder = [](const Item& item) {
        GUI::showNotification(item.title, item.message, NotificationsManager::NotificationType::NOTIFICATION_OKAY);
    };
    presenterCallbacks_.showAlarm = [this](const Item& item) {
        const auto snoozeMinutes = std::max(
            1,
            GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::ALARM_SNOOZE_MINUTES));
        GUI::showAlarmModal(
            "Alarm",
            item.message.empty() ? item.title : item.message,
            [this, id = item.id]() {
                acknowledge(id);
            },
            [this, id = item.id, snoozeMinutes]() {
                snooze(id, static_cast<int64_t>(snoozeMinutes) * 60LL * 1000LL);
            });
    };
    presenterCallbacks_.startAlarmSound = []() {
        AudioOutput::playFileAsync(defaultAlertSoundPath(), static_cast<float>(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::ALARM_SOUND_VOLUME)));
    };
    presenterCallbacks_.stopAlarmSound = []() {};
    presenterCallbacks_.playReminderSound = []() {
        AudioOutput::playFileAsync(defaultAlertSoundPath(), static_cast<float>(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::NOTIFICATION_SOUND_VOLUME)));
    };
}

NotificationCenter::~NotificationCenter()
{
    stop();
}

void NotificationCenter::setScheduler(const std::shared_ptr<Scheduler>& scheduler)
{
    std::scoped_lock lock(mutex_);
    scheduler_ = scheduler;
}

void NotificationCenter::setPresenterCallbacks(PresenterCallbacks callbacks)
{
    std::scoped_lock lock(mutex_);
    presenterCallbacks_ = std::move(callbacks);
}

void NotificationCenter::setSharedInstance(const std::shared_ptr<NotificationCenter>& instance)
{
    gSharedNotificationCenter = instance;
}

std::shared_ptr<NotificationCenter> NotificationCenter::sharedInstance()
{
    return gSharedNotificationCenter.lock();
}

void NotificationCenter::ensureSchema() const
{
    try {
        runNotificationDbTask([&](Database* db) {
            if (!db) return false;
            const std::vector<std::pair<std::string, std::string>> requiredColumns = {
                { "kind", "TEXT NOT NULL DEFAULT 'notification'" },
                { "created_at", "TEXT NOT NULL DEFAULT '0'" },
                { "scheduled_for", "TEXT NOT NULL DEFAULT ''" },
                { "delivered_at", "TEXT NOT NULL DEFAULT ''" },
                { "acknowledged", "INTEGER NOT NULL DEFAULT 0" },
                { "active", "INTEGER NOT NULL DEFAULT 0" },
                { "priority", "TEXT NOT NULL DEFAULT 'normal'" },
                { "repeat_rule", "TEXT NOT NULL DEFAULT 'none'" },
                { "metadata", "TEXT NOT NULL DEFAULT '{}'" }
            };
            for (const auto& [name, type] : requiredColumns) {
                if (!db->columnExists(DB_NS::TableNames::NOTIFICATIONS, name)) {
                    db->execute("ALTER TABLE " + DB_NS::TableNames::NOTIFICATIONS + " ADD COLUMN " + name + " " + type + ";");
                }
            }
            return true;
        });
    } catch (const std::exception& e) {
        CubeLog::error(std::string("NotificationCenter: failed to ensure schema: ") + e.what());
    } catch (...) {
        CubeLog::error("NotificationCenter: failed to ensure schema");
    }
}

void NotificationCenter::start()
{
    {
        std::scoped_lock lock(mutex_);
        if (started_) {
            return;
        }
        started_ = true;
    }
    ensureSchema();
    reloadScheduledItems();
}

void NotificationCenter::stop()
{
    std::vector<long> ids;
    {
        std::scoped_lock lock(mutex_);
        if (!started_) {
            return;
        }
        started_ = false;
        for (const auto& [id, handle] : scheduledHandles_) {
            (void)handle;
            ids.push_back(id);
        }
    }
    for (long id : ids) {
        unscheduleItem(id);
    }
    stopAlarmLoop();
}

long NotificationCenter::createNotification(
    const std::string& title,
    const std::string& message,
    const std::string& source,
    const std::string& priority,
    const nlohmann::json& metadata,
    bool displayNow)
{
    ensureSchema();
    if (!CubeDB::getDBManager()) {
        CubeLog::error("NotificationCenter: createNotification called before CubeDB manager was initialized");
        return -1;
    }
    Item item;
    item.kind = Kind::NOTIFICATION;
    item.title = title.empty() ? "Notification" : title;
    item.message = message;
    item.createdAtEpochMs = nowEpochMs();
    item.timeLegacyEpochMs = item.createdAtEpochMs;
    item.deliveredAtEpochMs = displayNow ? item.createdAtEpochMs : 0;
    item.source = source;
    item.priority = priority;
    item.active = false;
    item.metadata = metadata;

    long id = -1;
    try {
        id = runNotificationDbTask([&](Database* db) {
            std::vector<DB_NS::Table_Entry> entries = {
                { "kind", kindToString(item.kind) },
                { "title", item.title },
                { "message", item.message },
                { "time", std::to_string(item.timeLegacyEpochMs) },
                { "created_at", std::to_string(item.createdAtEpochMs) },
                { "scheduled_for", "" },
                { "delivered_at", epochString(item.deliveredAtEpochMs) },
                { "source", item.source },
                { "read", item.read ? "1" : "0" },
                { "acknowledged", "0" },
                { "active", "0" },
                { "priority", item.priority },
                { "repeat_rule", "none" },
                { "metadata", item.metadata.dump() }
            };
            return db ? db->insertData(DB_NS::TableNames::NOTIFICATIONS, entries) : -1L;
        });
    } catch (const std::exception& e) {
        CubeLog::error(std::string("NotificationCenter: failed to create notification: ") + e.what());
        return -1;
    }
    if (displayNow && id > 0) {
        item.id = id;
        presentItem(item);
    }
    return id;
}

long NotificationCenter::createReminder(
    const std::string& title,
    const std::string& message,
    int64_t scheduledForEpochMs,
    RepeatRule repeatRule,
    int64_t repeatIntervalSeconds,
    const std::string& source,
    const nlohmann::json& metadata)
{
    ensureSchema();
    if (!CubeDB::getDBManager()) {
        CubeLog::error("NotificationCenter: createReminder called before CubeDB manager was initialized");
        return -1;
    }
    Item item;
    item.kind = Kind::REMINDER;
    item.title = title.empty() ? "Reminder" : title;
    item.message = message.empty() ? item.title : message;
    item.createdAtEpochMs = nowEpochMs();
    item.timeLegacyEpochMs = scheduledForEpochMs;
    item.scheduledForEpochMs = scheduledForEpochMs;
    item.source = source;
    item.active = true;
    item.priority = "normal";
    item.repeatRule = repeatRule;
    item.repeatIntervalSeconds = repeatIntervalSeconds;
    item.metadata = metadata;

    long id = -1;
    try {
        id = runNotificationDbTask([&](Database* db) {
            std::vector<DB_NS::Table_Entry> entries = {
                { "kind", kindToString(item.kind) },
                { "title", item.title },
                { "message", item.message },
                { "time", std::to_string(item.timeLegacyEpochMs) },
                { "created_at", std::to_string(item.createdAtEpochMs) },
                { "scheduled_for", std::to_string(item.scheduledForEpochMs) },
                { "delivered_at", "" },
                { "source", item.source },
                { "read", "0" },
                { "acknowledged", "0" },
                { "active", "1" },
                { "priority", item.priority },
                { "repeat_rule", repeatRuleToString(item.repeatRule, item.repeatIntervalSeconds) },
                { "metadata", item.metadata.dump() }
            };
            return db ? db->insertData(DB_NS::TableNames::NOTIFICATIONS, entries) : -1L;
        });
    } catch (const std::exception& e) {
        CubeLog::error(std::string("NotificationCenter: failed to create reminder: ") + e.what());
        return -1;
    }
    if (id > 0) {
        item.id = id;
        scheduleItem(item);
    }
    return id;
}

long NotificationCenter::createAlarm(
    const std::string& title,
    const std::string& message,
    int64_t scheduledForEpochMs,
    RepeatRule repeatRule,
    int64_t repeatIntervalSeconds,
    const std::string& source,
    const nlohmann::json& metadata)
{
    ensureSchema();
    if (!CubeDB::getDBManager()) {
        CubeLog::error("NotificationCenter: createAlarm called before CubeDB manager was initialized");
        return -1;
    }
    Item item;
    item.kind = Kind::ALARM;
    item.title = title.empty() ? "Alarm" : title;
    item.message = message.empty() ? item.title : message;
    item.createdAtEpochMs = nowEpochMs();
    item.timeLegacyEpochMs = scheduledForEpochMs;
    item.scheduledForEpochMs = scheduledForEpochMs;
    item.source = source;
    item.active = true;
    item.priority = "high";
    item.repeatRule = repeatRule;
    item.repeatIntervalSeconds = repeatIntervalSeconds;
    item.metadata = metadata;

    long id = -1;
    try {
        id = runNotificationDbTask([&](Database* db) {
            std::vector<DB_NS::Table_Entry> entries = {
                { "kind", kindToString(item.kind) },
                { "title", item.title },
                { "message", item.message },
                { "time", std::to_string(item.timeLegacyEpochMs) },
                { "created_at", std::to_string(item.createdAtEpochMs) },
                { "scheduled_for", std::to_string(item.scheduledForEpochMs) },
                { "delivered_at", "" },
                { "source", item.source },
                { "read", "0" },
                { "acknowledged", "0" },
                { "active", "1" },
                { "priority", item.priority },
                { "repeat_rule", repeatRuleToString(item.repeatRule, item.repeatIntervalSeconds) },
                { "metadata", item.metadata.dump() }
            };
            return db ? db->insertData(DB_NS::TableNames::NOTIFICATIONS, entries) : -1L;
        });
    } catch (const std::exception& e) {
        CubeLog::error(std::string("NotificationCenter: failed to create alarm: ") + e.what());
        return -1;
    }
    if (id > 0) {
        item.id = id;
        scheduleItem(item);
    }
    return id;
}

std::vector<NotificationCenter::Item> NotificationCenter::listRecent(size_t limit) const
{
    ensureSchema();
    try {
        auto rows = runNotificationDbTask([&](Database* db) {
            return db ? db->selectData(DB_NS::TableNames::NOTIFICATIONS, notificationColumns()) : std::vector<std::vector<std::string>> {};
        });
        std::vector<Item> items;
        for (const auto& row : rows) {
            if (auto item = rowToItem(row)) {
                items.push_back(*item);
            }
        }
        std::sort(items.begin(), items.end(), [](const Item& lhs, const Item& rhs) {
            return recentSortKey(lhs) > recentSortKey(rhs);
        });
        if (items.size() > limit) {
            items.resize(limit);
        }
        return items;
    } catch (...) {
        return {};
    }
}

std::vector<NotificationCenter::Item> NotificationCenter::listUpcomingReminders(size_t limit) const
{
    ensureSchema();
    try {
        auto rows = runNotificationDbTask([&](Database* db) {
            return db ? db->selectData(
                            DB_NS::TableNames::NOTIFICATIONS,
                            notificationColumns(),
                            { DB_NS::Predicate { "kind", "reminder" }, DB_NS::Predicate { "active", "1" } })
                      : std::vector<std::vector<std::string>> {};
        });
        std::vector<Item> items;
        for (const auto& row : rows) {
            if (auto item = rowToItem(row); item.has_value() && item->scheduledForEpochMs > 0) {
                items.push_back(*item);
            }
        }
        std::sort(items.begin(), items.end(), [](const Item& lhs, const Item& rhs) {
            return lhs.scheduledForEpochMs < rhs.scheduledForEpochMs;
        });
        if (items.size() > limit) {
            items.resize(limit);
        }
        return items;
    } catch (...) {
        return {};
    }
}

std::vector<NotificationCenter::Item> NotificationCenter::listActiveAlarms(size_t limit) const
{
    ensureSchema();
    try {
        auto rows = runNotificationDbTask([&](Database* db) {
            return db ? db->selectData(
                            DB_NS::TableNames::NOTIFICATIONS,
                            notificationColumns(),
                            { DB_NS::Predicate { "kind", "alarm" }, DB_NS::Predicate { "active", "1" } })
                      : std::vector<std::vector<std::string>> {};
        });
        std::vector<Item> items;
        for (const auto& row : rows) {
            if (auto item = rowToItem(row)) {
                items.push_back(*item);
            }
        }
        std::sort(items.begin(), items.end(), [](const Item& lhs, const Item& rhs) {
            return activeAlarmSortKey(lhs) < activeAlarmSortKey(rhs);
        });
        if (items.size() > limit) {
            items.resize(limit);
        }
        return items;
    } catch (...) {
        return {};
    }
}

std::optional<NotificationCenter::Item> NotificationCenter::getItem(long id) const
{
    ensureSchema();
    try {
        auto rows = runNotificationDbTask([&](Database* db) {
            return db ? db->selectData(DB_NS::TableNames::NOTIFICATIONS, notificationColumns(), { DB_NS::Predicate { "id", std::to_string(id) } }) : std::vector<std::vector<std::string>> {};
        });
        if (rows.empty()) {
            return std::nullopt;
        }
        return rowToItem(rows.front());
    } catch (...) {
        return std::nullopt;
    }
}

bool NotificationCenter::updateItem(const Item& item)
{
    ensureSchema();
    try {
        return runNotificationDbTask([&](Database* db) {
            if (!db) return false;
            return db->updateData(
                DB_NS::TableNames::NOTIFICATIONS,
                {
                    "kind",
                    "title",
                    "message",
                    "time",
                    "created_at",
                    "scheduled_for",
                    "delivered_at",
                    "source",
                    "read",
                    "acknowledged",
                    "active",
                    "priority",
                    "repeat_rule",
                    "metadata"
                },
                {
                    kindToString(item.kind),
                    item.title,
                    item.message,
                    std::to_string(item.timeLegacyEpochMs),
                    std::to_string(item.createdAtEpochMs),
                    epochString(item.scheduledForEpochMs),
                    epochString(item.deliveredAtEpochMs),
                    item.source,
                    item.read ? "1" : "0",
                    item.acknowledged ? "1" : "0",
                    item.active ? "1" : "0",
                    item.priority,
                    repeatRuleToString(item.repeatRule, item.repeatIntervalSeconds),
                    item.metadata.dump()
                },
                { DB_NS::Predicate { "id", std::to_string(item.id) } });
        });
    } catch (...) {
        return false;
    }
}

void NotificationCenter::scheduleItem(const Item& item)
{
    std::shared_ptr<Scheduler> scheduler;
    {
        std::scoped_lock lock(mutex_);
        scheduler = scheduler_;
    }
    if (!scheduler || item.scheduledForEpochMs <= 0 || !item.active) {
        return;
    }

    auto reg = std::make_shared<Intent>(
        "__notifications.deliver." + std::to_string(item.id),
        [this, id = item.id](const Parameters&, Intent) {
            deliverItem(id);
        });

    ScheduledTask task(reg, TimePoint(std::chrono::milliseconds(item.scheduledForEpochMs)));
    const auto handle = scheduler->addTask(task);
    std::scoped_lock lock(mutex_);
    scheduledHandles_[item.id] = handle;
}

void NotificationCenter::unscheduleItem(long id)
{
    std::shared_ptr<Scheduler> scheduler;
    uint32_t handle = 0;
    {
        std::scoped_lock lock(mutex_);
        scheduler = scheduler_;
        auto it = scheduledHandles_.find(id);
        if (it == scheduledHandles_.end()) {
            return;
        }
        handle = it->second;
        scheduledHandles_.erase(it);
    }
    if (scheduler && handle != 0) {
        scheduler->removeTask(handle);
    }
}

int64_t NotificationCenter::computeNextScheduledFor(const Item& item, int64_t baseEpochMs) const
{
    switch (item.repeatRule) {
    case RepeatRule::DAILY:
        return baseEpochMs + (24LL * 60LL * 60LL * 1000LL);
    case RepeatRule::WEEKLY:
        return baseEpochMs + (7LL * 24LL * 60LL * 60LL * 1000LL);
    case RepeatRule::CUSTOM_SECONDS:
        return baseEpochMs + (item.repeatIntervalSeconds * 1000LL);
    case RepeatRule::NONE:
        break;
    }
    return 0;
}

void NotificationCenter::reloadScheduledItems()
{
    constexpr int64_t staleAlarmGraceMs = 5LL * 60LL * 1000LL;
    stopAlarmLoop();
    const auto now = nowEpochMs();
    const auto reminders = listUpcomingReminders(250);
    const auto alarms = listActiveAlarms(250);
    for (const auto& item : reminders) {
        if (item.scheduledForEpochMs > now) {
            scheduleItem(item);
        } else {
            auto updated = item;
            const auto next = computeNextScheduledFor(updated, updated.scheduledForEpochMs);
            if (next > 0) {
                updated.scheduledForEpochMs = next;
                updated.active = true;
                updated.acknowledged = false;
                updateItem(updated);
                scheduleItem(updated);
                CubeLog::info("NotificationCenter: rescheduled stale reminder " + std::to_string(updated.id));
            } else {
                updated.active = false;
                updated.acknowledged = true;
                updated.read = true;
                updated.scheduledForEpochMs = 0;
                updateItem(updated);
                CubeLog::info("NotificationCenter: archived stale reminder " + std::to_string(updated.id));
            }
        }
    }
    for (const auto& item : alarms) {
        if (item.deliveredAtEpochMs > 0 && item.active && !item.acknowledged) {
            if (item.deliveredAtEpochMs + staleAlarmGraceMs < now) {
                auto updated = item;
                if (updated.repeatRule != RepeatRule::NONE) {
                    auto next = computeNextScheduledFor(updated, updated.deliveredAtEpochMs);
                    while (next > 0 && next <= now) {
                        next = computeNextScheduledFor(updated, next);
                    }
                    if (next > 0) {
                        updated.scheduledForEpochMs = next;
                        updated.active = true;
                        updated.acknowledged = false;
                        updateItem(updated);
                        scheduleItem(updated);
                        CubeLog::info("NotificationCenter: rescheduled stale active alarm " + std::to_string(updated.id));
                        continue;
                    }
                }

                updated.active = false;
                updated.acknowledged = true;
                updated.read = true;
                updated.scheduledForEpochMs = 0;
                updateItem(updated);
                CubeLog::info("NotificationCenter: archived stale active alarm " + std::to_string(updated.id));
                continue;
            }
            {
                std::scoped_lock lock(mutex_);
                activeAlarmId_ = item.id;
            }
            presentItem(item);
            startAlarmLoop(item.id);
            continue;
        }
        if (item.scheduledForEpochMs > now) {
            scheduleItem(item);
        } else {
            auto updated = item;
            auto next = computeNextScheduledFor(updated, updated.scheduledForEpochMs);
            while (next > 0 && next <= now) {
                next = computeNextScheduledFor(updated, next);
            }
            if (next > 0) {
                updated.scheduledForEpochMs = next;
                updated.active = true;
                updated.acknowledged = false;
                updateItem(updated);
                scheduleItem(updated);
                CubeLog::info("NotificationCenter: rescheduled stale scheduled alarm " + std::to_string(updated.id));
            } else {
                updated.active = false;
                updated.acknowledged = true;
                updated.read = true;
                updated.scheduledForEpochMs = 0;
                updateItem(updated);
                CubeLog::info("NotificationCenter: archived stale scheduled alarm " + std::to_string(updated.id));
            }
        }
    }
}

void NotificationCenter::presentItem(const Item& item)
{
    PresenterCallbacks callbacks;
    {
        std::scoped_lock lock(mutex_);
        callbacks = presenterCallbacks_;
    }

    switch (item.kind) {
    case Kind::NOTIFICATION:
        if (callbacks.showNotification) callbacks.showNotification(item);
        break;
    case Kind::REMINDER:
        if (callbacks.playReminderSound) callbacks.playReminderSound();
        if (callbacks.showReminder) callbacks.showReminder(item);
        break;
    case Kind::ALARM:
        if (callbacks.showAlarm) callbacks.showAlarm(item);
        break;
    }
}

std::jthread NotificationCenter::takeAlarmPlaybackThreadLocked()
{
    if (!alarmPlaybackThread_.joinable()) {
        return {};
    }

    alarmPlaybackThread_.request_stop();
    return std::move(alarmPlaybackThread_);
}

void NotificationCenter::startAlarmLoop(long id)
{
    const auto generation = alarmPlaybackGeneration_.fetch_add(1, std::memory_order_relaxed) + 1;
    std::jthread previousThread;
    {
        std::scoped_lock lock(mutex_);
        previousThread = takeAlarmPlaybackThreadLocked();
    }
    if (previousThread.joinable()) {
        previousThread.join();
    }

    std::scoped_lock lock(mutex_);
    alarmPlaybackThread_ = std::jthread([this, id, generation](std::stop_token stopToken) {
        while (!stopToken.stop_requested()
               && alarmPlaybackGeneration_.load(std::memory_order_relaxed) == generation) {
            auto item = getItem(id);
            if (!item || !item->active || item->acknowledged) {
                break;
            }
            PresenterCallbacks callbacks;
            {
                std::scoped_lock lock(mutex_);
                callbacks = presenterCallbacks_;
            }
            if (callbacks.startAlarmSound) {
                callbacks.startAlarmSound();
            }
            for (int i = 0; i < 20 && !stopToken.stop_requested(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        PresenterCallbacks callbacks;
        {
            std::scoped_lock lock(mutex_);
            callbacks = presenterCallbacks_;
        }
        if (callbacks.stopAlarmSound) {
            callbacks.stopAlarmSound();
        }
    });
}

void NotificationCenter::stopAlarmLoop()
{
    alarmPlaybackGeneration_.fetch_add(1, std::memory_order_relaxed);
    std::jthread alarmThread;
    PresenterCallbacks callbacks;
    {
        std::scoped_lock lock(mutex_);
        activeAlarmId_ = -1;
        callbacks = presenterCallbacks_;
        alarmThread = takeAlarmPlaybackThreadLocked();
    }
    if (alarmThread.joinable()) {
        alarmThread.join();
    }
    if (callbacks.stopAlarmSound) {
        callbacks.stopAlarmSound();
    }
}

void NotificationCenter::deliverItem(long id)
{
    auto itemOpt = getItem(id);
    if (!itemOpt.has_value()) {
        return;
    }
    auto item = *itemOpt;
    unscheduleItem(id);

    if (item.kind == Kind::ALARM) {
        std::scoped_lock lock(mutex_);
        if (activeAlarmId_ != -1 && activeAlarmId_ != id) {
            item.scheduledForEpochMs = nowEpochMs() + 30000;
            updateItem(item);
            scheduleItem(item);
            return;
        }
        activeAlarmId_ = id;
    }

    const auto deliveredAt = nowEpochMs();
    item.deliveredAtEpochMs = deliveredAt;
    item.read = false;
    item.acknowledged = false;
    if (item.kind == Kind::REMINDER) {
        const auto next = computeNextScheduledFor(item, deliveredAt);
        if (next > 0) {
            item.scheduledForEpochMs = next;
            item.active = true;
        } else {
            item.scheduledForEpochMs = 0;
            item.active = false;
        }
    } else if (item.kind == Kind::ALARM) {
        item.active = true;
    } else {
        item.active = false;
    }

    updateItem(item);
    presentItem(item);

    if (item.kind == Kind::ALARM) {
        startAlarmLoop(item.id);
    }
    if (item.kind == Kind::REMINDER && item.active && item.scheduledForEpochMs > 0) {
        scheduleItem(item);
    }
}

bool NotificationCenter::acknowledge(long id)
{
    auto itemOpt = getItem(id);
    if (!itemOpt.has_value()) {
        return false;
    }
    auto item = *itemOpt;
    item.acknowledged = true;
    item.read = true;
    const auto next = computeNextScheduledFor(item, nowEpochMs());
    if (item.kind == Kind::ALARM && next > 0) {
        item.scheduledForEpochMs = next;
        item.active = true;
    } else {
        item.active = false;
        item.scheduledForEpochMs = 0;
    }
    if (updateItem(item)) {
        stopAlarmLoop();
        if (item.active && item.scheduledForEpochMs > 0) {
            scheduleItem(item);
        }
        return true;
    }
    return false;
}

bool NotificationCenter::dismiss(long id)
{
    auto itemOpt = getItem(id);
    if (!itemOpt.has_value()) {
        return false;
    }
    auto item = *itemOpt;
    item.active = false;
    item.read = true;
    item.acknowledged = true;
    item.scheduledForEpochMs = 0;
    if (updateItem(item)) {
        unscheduleItem(id);
        if (item.kind == Kind::ALARM) {
            stopAlarmLoop();
        }
        return true;
    }
    return false;
}

bool NotificationCenter::snooze(long id, int64_t delayMs)
{
    auto itemOpt = getItem(id);
    if (!itemOpt.has_value()) {
        return false;
    }
    auto item = *itemOpt;
    item.scheduledForEpochMs = nowEpochMs() + std::max<int64_t>(delayMs, 1000);
    item.active = true;
    item.acknowledged = false;
    item.read = false;
    if (updateItem(item)) {
        if (item.kind == Kind::ALARM) {
            stopAlarmLoop();
        }
        scheduleItem(item);
        return true;
    }
    return false;
}

bool NotificationCenter::markRead(long id, bool read)
{
    auto itemOpt = getItem(id);
    if (!itemOpt.has_value()) {
        return false;
    }
    auto item = *itemOpt;
    item.read = read;
    return updateItem(item);
}

std::string NotificationCenter::recentSummary(size_t limit) const
{
    const auto items = listRecent(limit);
    if (items.empty()) {
        return "No recent notifications.";
    }
    std::ostringstream out;
    for (const auto& item : items) {
        out << "- [" << kindToString(item.kind) << "] " << item.title;
        if (!item.message.empty() && item.message != item.title) {
            out << ": " << item.message;
        }
        if (item.scheduledForEpochMs > 0) {
            out << " (" << formatEpochMs(item.scheduledForEpochMs) << ")";
        }
        out << "\n";
    }
    return trim(out.str());
}

nlohmann::json NotificationCenter::createReminderFromTranscript(const std::string& transcript)
{
    const auto spec = parseReminderTranscript(transcript);
    if (spec.scheduledForEpochMs <= 0 || spec.message.empty()) {
        return nlohmann::json({ { "status", "error" }, { "error", "unable_to_parse_reminder" } });
    }
    const auto id = createReminder(spec.title, spec.message, spec.scheduledForEpochMs, spec.repeatRule, spec.repeatIntervalSeconds);
    return nlohmann::json({
        { "status", id > 0 ? "ok" : "error" },
        { "notificationId", id },
        { "scheduledForEpochMs", spec.scheduledForEpochMs },
        { "summary", "Reminder set for " + formatEpochMs(spec.scheduledForEpochMs) + "." }
    });
}

nlohmann::json NotificationCenter::createReminderFromArgs(const nlohmann::json& args)
{
    const auto scheduledForEpochMs = scheduledForEpochMsFromArgs(args);
    if (!scheduledForEpochMs.has_value()) {
        return nlohmann::json({ { "status", "error" }, { "error", "scheduled_for_required" } });
    }

    const auto title = args.value("title", std::string("Reminder"));
    const auto message = args.value("message", title);
    int64_t repeatSeconds = 0;
    const auto repeatRule = repeatRuleFromString(args.value("repeatRule", std::string("none")), repeatSeconds);
    if (args.contains("repeatSeconds") && args["repeatSeconds"].is_number_integer()) {
        repeatSeconds = args["repeatSeconds"].get<int64_t>();
    }

    const auto id = createReminder(title, message, *scheduledForEpochMs, repeatRule, repeatSeconds);
    return nlohmann::json({
        { "status", id > 0 ? "ok" : "error" },
        { "notificationId", id },
        { "scheduledForEpochMs", *scheduledForEpochMs },
        { "summary", "Reminder set for " + formatEpochMs(*scheduledForEpochMs) + "." }
    });
}

nlohmann::json NotificationCenter::createAlarmFromTranscript(const std::string& transcript)
{
    const auto spec = parseAlarmTranscript(transcript);
    if (spec.scheduledForEpochMs <= 0) {
        return nlohmann::json({ { "status", "error" }, { "error", "unable_to_parse_alarm" } });
    }
    const auto id = createAlarm(spec.title, spec.message, spec.scheduledForEpochMs, spec.repeatRule, spec.repeatIntervalSeconds);
    return nlohmann::json({
        { "status", id > 0 ? "ok" : "error" },
        { "notificationId", id },
        { "scheduledForEpochMs", spec.scheduledForEpochMs },
        { "summary", "Alarm set for " + formatEpochMs(spec.scheduledForEpochMs) + "." }
    });
}

nlohmann::json NotificationCenter::createAlarmFromArgs(const nlohmann::json& args)
{
    const auto scheduledForEpochMs = scheduledForEpochMsFromArgs(args);
    if (!scheduledForEpochMs.has_value()) {
        return nlohmann::json({ { "status", "error" }, { "error", "scheduled_for_required" } });
    }

    const auto title = args.value("title", std::string("Alarm"));
    const auto message = args.value("message", title);
    int64_t repeatSeconds = 0;
    const auto repeatRule = repeatRuleFromString(args.value("repeatRule", std::string("none")), repeatSeconds);
    if (args.contains("repeatSeconds") && args["repeatSeconds"].is_number_integer()) {
        repeatSeconds = args["repeatSeconds"].get<int64_t>();
    }

    const auto id = createAlarm(title, message, *scheduledForEpochMs, repeatRule, repeatSeconds);
    return nlohmann::json({
        { "status", id > 0 ? "ok" : "error" },
        { "notificationId", id },
        { "scheduledForEpochMs", *scheduledForEpochMs },
        { "summary", "Alarm set for " + formatEpochMs(*scheduledForEpochMs) + "." }
    });
}

nlohmann::json NotificationCenter::listUpcomingRemindersResult() const
{
    const auto items = listUpcomingReminders();
    std::vector<std::string> summaries;
    for (const auto& item : items) {
        summaries.push_back(item.title + " at " + formatEpochMs(item.scheduledForEpochMs));
    }
    return nlohmann::json({
        { "status", "ok" },
        { "count", items.size() },
        { "items", [&]() {
             nlohmann::json out = nlohmann::json::array();
             for (const auto& item : items) {
                 out.push_back(item.toJson());
             }
             return out;
         }() },
        { "summary", items.empty() ? "You have no upcoming reminders." : ("Upcoming reminders: " + joinSummaries(summaries)) }
    });
}

nlohmann::json NotificationCenter::listActiveAlarmsResult() const
{
    const auto items = listActiveAlarms();
    std::vector<std::string> summaries;
    for (const auto& item : items) {
        summaries.push_back(item.title + " at " + formatEpochMs(item.scheduledForEpochMs > 0 ? item.scheduledForEpochMs : item.deliveredAtEpochMs));
    }
    return nlohmann::json({
        { "status", "ok" },
        { "count", items.size() },
        { "items", [&]() {
             nlohmann::json out = nlohmann::json::array();
             for (const auto& item : items) {
                 out.push_back(item.toJson());
             }
             return out;
         }() },
        { "summary", items.empty() ? "There are no active alarms." : ("Active alarms: " + joinSummaries(summaries)) }
    });
}

nlohmann::json NotificationCenter::cancelFromArgs(const nlohmann::json& args)
{
    if (args.is_object() && args.contains("notificationId") && args["notificationId"].is_number_integer()) {
        const auto id = args["notificationId"].get<long>();
        if (dismiss(id)) {
            return nlohmann::json({ { "status", "ok" }, { "summary", "Notification cancelled." } });
        }
        return nlohmann::json({ { "status", "error" }, { "error", "notification_not_found" } });
    }

    const auto kind = lowerAscii(args.value("kind", std::string("")));
    if (kind == "alarm") {
        const auto alarms = listActiveAlarms(1);
        if (!alarms.empty() && dismiss(alarms.front().id)) {
            return nlohmann::json({ { "status", "ok" }, { "summary", "Alarm cancelled." } });
        }
        return nlohmann::json({ { "status", "error" }, { "error", "no_alarm_found" } });
    }

    if (kind == "reminder") {
        const auto reminders = listUpcomingReminders(1);
        if (!reminders.empty() && dismiss(reminders.front().id)) {
            return nlohmann::json({ { "status", "ok" }, { "summary", "Reminder cancelled." } });
        }
        return nlohmann::json({ { "status", "error" }, { "error", "no_reminder_found" } });
    }

    const auto alarms = listActiveAlarms(1);
    if (!alarms.empty() && dismiss(alarms.front().id)) {
        return nlohmann::json({ { "status", "ok" }, { "summary", "Alarm cancelled." } });
    }
    const auto reminders = listUpcomingReminders(1);
    if (!reminders.empty() && dismiss(reminders.front().id)) {
        return nlohmann::json({ { "status", "ok" }, { "summary", "Reminder cancelled." } });
    }
    return nlohmann::json({ { "status", "error" }, { "error", "no_notification_found" } });
}

nlohmann::json NotificationCenter::cancelFromTranscript(const std::string& transcript)
{
    const auto lower = lowerAscii(transcript);
    if (lower.find("alarm") != std::string::npos) {
        const auto alarms = listActiveAlarms(1);
        if (!alarms.empty() && dismiss(alarms.front().id)) {
            return nlohmann::json({ { "status", "ok" }, { "summary", "Alarm cancelled." } });
        }
        return nlohmann::json({ { "status", "error" }, { "error", "no_alarm_found" } });
    }

    const auto reminders = listUpcomingReminders(1);
    if (!reminders.empty() && dismiss(reminders.front().id)) {
        return nlohmann::json({ { "status", "ok" }, { "summary", "Reminder cancelled." } });
    }
    return nlohmann::json({ { "status", "error" }, { "error", "no_reminder_found" } });
}

nlohmann::json NotificationCenter::snoozeFromArgs(const nlohmann::json& args)
{
    long targetId = -1;
    if (args.is_object() && args.contains("notificationId") && args["notificationId"].is_number_integer()) {
        targetId = args["notificationId"].get<long>();
    } else {
        const auto alarms = listActiveAlarms(1);
        if (alarms.empty()) {
            return nlohmann::json({ { "status", "error" }, { "error", "no_alarm_found" } });
        }
        targetId = alarms.front().id;
    }

    const auto delayMs = args.value("delayMs", 10LL * 60LL * 1000LL);
    if (snooze(targetId, delayMs)) {
        return nlohmann::json({
            { "status", "ok" },
            { "delayMs", delayMs },
            { "summary", "Alarm snoozed for " + std::to_string(delayMs / 60000LL) + " minutes." }
        });
    }
    return nlohmann::json({ { "status", "error" }, { "error", "snooze_failed" } });
}

nlohmann::json NotificationCenter::snoozeFromTranscript(const std::string& transcript)
{
    const auto alarms = listActiveAlarms(1);
    if (alarms.empty()) {
        return nlohmann::json({ { "status", "error" }, { "error", "no_alarm_found" } });
    }
    const auto delayMs = parseSnoozeDelayMs(transcript);
    if (snooze(alarms.front().id, delayMs)) {
        return nlohmann::json({
            { "status", "ok" },
            { "delayMs", delayMs },
            { "summary", "Alarm snoozed for " + std::to_string(delayMs / 60000LL) + " minutes." }
        });
    }
    return nlohmann::json({ { "status", "error" }, { "error", "snooze_failed" } });
}

HttpEndPointData_t NotificationCenter::getHttpEndpointData()
{
    HttpEndPointData_t data;

    auto parseBody = [](const httplib::Request& req) -> nlohmann::json {
        if (!(req.has_header("Content-Type") && req.get_header_value("Content-Type").find("application/json") != std::string::npos)) {
            throw std::runtime_error("Content-Type must be application/json");
        }
        return nlohmann::json::parse(req.body);
    };

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                const auto body = parseBody(req);
                const auto id = createNotification(
                    body.value("title", std::string("Notification")),
                    body.value("message", std::string("")),
                    body.value("source", std::string("local_api")),
                    body.value("priority", std::string("normal")),
                    body.value("metadata", nlohmann::json::object()),
                    body.value("displayNow", true));
                res.set_content(nlohmann::json({ { "success", id > 0 }, { "id", id } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (const std::exception& e) {
                res.set_content(nlohmann::json({ { "success", false }, { "message", e.what() } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "createNotification",
        nlohmann::json({ { "type", "object" } }),
        "Create and optionally display a generic notification"
    });

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                const auto body = parseBody(req);
                const auto scheduledForEpochMs = body.value("scheduledForEpochMs", static_cast<int64_t>(0));
                const auto repeatRaw = body.value("repeatRule", std::string("none"));
                int64_t repeatSeconds = 0;
                const auto repeatRule = repeatRuleFromString(repeatRaw, repeatSeconds);
                const auto id = createReminder(
                    body.value("title", std::string("Reminder")),
                    body.value("message", std::string("")),
                    scheduledForEpochMs,
                    repeatRule,
                    body.value("repeatSeconds", repeatSeconds),
                    body.value("source", std::string("local_api")),
                    body.value("metadata", nlohmann::json::object()));
                res.set_content(nlohmann::json({ { "success", id > 0 }, { "id", id } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (const std::exception& e) {
                res.set_content(nlohmann::json({ { "success", false }, { "message", e.what() } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "createReminder",
        nlohmann::json({ { "type", "object" } }),
        "Create a reminder"
    });

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                const auto body = parseBody(req);
                const auto scheduledForEpochMs = body.value("scheduledForEpochMs", static_cast<int64_t>(0));
                const auto repeatRaw = body.value("repeatRule", std::string("none"));
                int64_t repeatSeconds = 0;
                const auto repeatRule = repeatRuleFromString(repeatRaw, repeatSeconds);
                const auto id = createAlarm(
                    body.value("title", std::string("Alarm")),
                    body.value("message", std::string("Alarm")),
                    scheduledForEpochMs,
                    repeatRule,
                    body.value("repeatSeconds", repeatSeconds),
                    body.value("source", std::string("local_api")),
                    body.value("metadata", nlohmann::json::object()));
                res.set_content(nlohmann::json({ { "success", id > 0 }, { "id", id } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (const std::exception& e) {
                res.set_content(nlohmann::json({ { "success", false }, { "message", e.what() } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "createAlarm",
        nlohmann::json({ { "type", "object" } }),
        "Create an alarm"
    });

    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request&, httplib::Response& res) {
            nlohmann::json response;
            response["success"] = true;
            response["items"] = nlohmann::json::array();
            for (const auto& item : listRecent()) {
                response["items"].push_back(item.toJson());
            }
            res.set_content(response.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "listNotifications",
        nlohmann::json({ { "type", "object" } }),
        "List recent notifications"
    });

    auto stateMutationHandler = [this, parseBody](const httplib::Request& req, httplib::Response& res, const std::function<bool(long, const nlohmann::json&)>& fn) {
        try {
            const auto body = parseBody(req);
            const auto id = body.at("id").get<long>();
            const bool success = fn(id, body);
            res.set_content(nlohmann::json({ { "success", success } }).dump(), "application/json");
            return success ? EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "")
                           : EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Mutation failed");
        } catch (const std::exception& e) {
            res.set_content(nlohmann::json({ { "success", false }, { "message", e.what() } }).dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
        }
    };

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            return stateMutationHandler(req, res, [this](long id, const nlohmann::json&) { return acknowledge(id); });
        },
        "acknowledgeNotification",
        nlohmann::json({ { "type", "object" } }),
        "Acknowledge an alarm or reminder"
    });

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            return stateMutationHandler(req, res, [this](long id, const nlohmann::json&) { return dismiss(id); });
        },
        "dismissNotification",
        nlohmann::json({ { "type", "object" } }),
        "Dismiss a reminder or alarm"
    });

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            return stateMutationHandler(req, res, [this](long id, const nlohmann::json& body) {
                return snooze(id, body.value("delayMs", 10LL * 60LL * 1000LL));
            });
        },
        "snoozeAlarm",
        nlohmann::json({ { "type", "object" } }),
        "Snooze an alarm"
    });

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            return stateMutationHandler(req, res, [this](long id, const nlohmann::json& body) {
                return markRead(id, body.value("read", true));
            });
        },
        "markNotificationRead",
        nlohmann::json({ { "type", "object" } }),
        "Mark a notification as read or unread"
    });

    return data;
}
