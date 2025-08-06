#pragma once
#include "../database/cubeDB.h"
#include "utils.h"
#ifndef LOGGER_H
#include <logger.h>
#endif
#ifndef API_I_H
#include "../api/api.h"
#endif
#include "cubeWhisper.h"
#include "nlohmann/json.hpp"
#include "remoteServer.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../api/autoRegister.h"
#include "../audio/audioManager.h"
#include "../threadsafeQueue.h"
#include "globalSettings.h"
#include "httplib.h"
#include "personalityManager.h"
#include "intentRegistry.h"
#include "functionRegistry.h"
#define SCHEDULER_THREAD_SLEEP_MS 100

namespace DecisionEngine {

class ScheduledTask {
public:
    struct RepeatInterval {
        enum class Interval {
            REPEAT_NONE_ONE_SHOT,
            REPEAT_DAYS,
            REPEAT_WEEKS,
            REPEAT_MONTHS,
            REPEAT_YEARS,
            REPEAT_CUSTOM // Custom interval. Measured from start time in seconds.
        };
        Interval interval;
        uint32_t value; // The value of the interval, e.g. 1 for 1 day, 2 for 2 weeks, etc.
        RepeatInterval(Interval interval = Interval::REPEAT_NONE_ONE_SHOT, uint32_t value = 0)
            : interval(interval), value(value) {}
        bool operator==(const RepeatInterval& other) const
        {
            return interval == other.interval && value == other.value;
        }
        bool operator!=(const RepeatInterval& other) const
        {
            return !(*this == other);
        }
        // operator to get the time in seconds for the interval. If it is a custom interval, it will return the value as is.
        uint32_t toSeconds() const
        {
            switch (interval) {
            case Interval::REPEAT_NONE_ONE_SHOT:
                return 0; // One-shot, no repeat
            case Interval::REPEAT_DAYS:
                return value * 24 * 60 * 60; // Days to seconds
            case Interval::REPEAT_WEEKS:
                return value * 7 * 24 * 60 * 60; // Weeks to seconds
            case Interval::REPEAT_MONTHS:
                return value * 30 * 24 * 60 * 60; // Months to seconds (approximation)
            case Interval::REPEAT_YEARS:
                return value * 365 * 24 * 60 * 60; // Years to seconds (approximation)
            case Interval::REPEAT_CUSTOM:
                return value; // Custom interval, return the value as is
            }
            return 0; // Default case, should not happen
        }
    };
    // A handle to a scheduled or schedule-able task
    using ScheduledTaskHandle = uint32_t;    
    struct ScheduleType {
        // The time when the task is scheduled to run
        TimePoint time;
        // The repeat interval of the task, if it is a repeating task, this will be the interval.
        RepeatInterval repeat;
        // The end time of the schedule, if it is a repeating schedule, this will be the time when the schedule ends.
        // If it is a one-shot schedule, this will be the same as the time.
        TimePoint endTime;
    };

    ScheduledTask() = default;
    ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time);
    ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatInterval& repeat);
    ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatInterval& repeat, const TimePoint& endTime);
    ScheduledTask(const std::shared_ptr<Intent>& intent, const ScheduleType& schedule) 
        : intent(intent), schedule(schedule)
    {
        this->handle = nextHandle++;
        this->enabled = true;
    }
    const ScheduleType& getSchedule() const;
    const std::shared_ptr<Intent>& getIntent() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    ScheduledTaskHandle getHandle() const;
    void executeIntent();

private:
    ScheduleType schedule = { TimePoint(), { RepeatInterval::Interval::REPEAT_NONE_ONE_SHOT, 0 }, TimePoint() };
    std::shared_ptr<Intent> intent;
    bool enabled = false;
    static ScheduledTaskHandle nextHandle;
    ScheduledTaskHandle handle = 0;
    unsigned int repeatCount = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

class Scheduler : public AutoRegisterAPI<Scheduler> {
public:
    // A vector of scheduled tasks
    using ScheduledTaskList = std::vector<ScheduledTask>;

    Scheduler();
    ~Scheduler();
    void start();
    void stop();
    void pause();
    void resume();
    void restart();
    void setIntentRecognition(std::shared_ptr<I_IntentRecognition> intentRecognition);
    uint32_t addTask(const ScheduledTask& task);
    void removeTask(const std::shared_ptr<Intent>& intent);
    void removeTask(const std::string& intentName);
    void removeTask(uint32_t taskHandle);

    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    std::string getInterfaceName() const override;

private:
    std::shared_ptr<I_IntentRecognition> intentRecognition;
    ScheduledTaskList scheduledTasks;
    std::jthread* schedulerThread;
    std::mutex schedulerMutex;
    std::condition_variable schedulerCV;
    bool schedulerRunning = false;
    bool schedulerPaused = false;
    void schedulerThreadFunction(std::stop_token st);
};

}