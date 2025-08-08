// Scheduler: central time-based orchestrator for user intents
//
// Responsibilities
// - Persist and manage a set of ScheduledTask entries (one-shot or repeating)
// - Wake on an internal cadence (SCHEDULER_THREAD_SLEEP_MS) to evaluate due tasks
// - Execute the bound Intent when a task becomes due
// - Expose a lightweight HTTP API for control (start/stop/pause/resume) and CRUD on tasks
//
// Threading model
// - A std::jthread runs schedulerThreadFunction, sleeping between evaluations
// - Public mutators guard shared state with a mutex + condition_variable
// - Intent execution happens on the scheduler thread; long work should be offloaded by the intent itself
//
// Time semantics
// - ScheduleType::time uses system_clock; repeats are expressed in seconds via RepeatInterval::toSeconds
// - REPEAT_CUSTOM interprets value as seconds from the last start time
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
    // RepeatInterval: declarative repeat cadence for a task.
    // Use REPEAT_NONE_ONE_SHOT for one-time execution. REPEAT_* map to seconds via toSeconds().
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
        uint32_t value; // Interval multiplier (e.g., 1 day, 2 weeks, or raw seconds for CUSTOM)
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
        // Convert to seconds. CUSTOM returns value unmodified.
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
        // Absolute time when the task becomes due
        TimePoint time;
        // Repeat interval; zero means one-shot
        RepeatInterval repeat;
        // Optional end time for repeating schedules; ignored for one-shots
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
    const ScheduleType& getSchedule() const;        // Inspect schedule metadata
    const std::shared_ptr<Intent>& getIntent() const;// Access bound intent
    bool isEnabled() const;                          // Whether the task is considered by the scheduler
    void setEnabled(bool enabled);                   // Enable/disable without removing from list
    ScheduledTaskHandle getHandle() const;           // Stable identifier for CRUD operations
    void executeIntent();                            // Invoke the bound intent (increments repeatCount)

private:
    ScheduleType schedule = { TimePoint(), { RepeatInterval::Interval::REPEAT_NONE_ONE_SHOT, 0 }, TimePoint() };
    std::shared_ptr<Intent> intent;
    bool enabled = false;
    static ScheduledTaskHandle nextHandle;
    ScheduledTaskHandle handle = 0;
    unsigned int repeatCount = 0; // Number of times this task has executed
};

/////////////////////////////////////////////////////////////////////////////////////

class Scheduler : public AutoRegisterAPI<Scheduler> {
public:
    // A vector of scheduled tasks
    using ScheduledTaskList = std::vector<ScheduledTask>;

    // Lifecycle
    Scheduler();
    ~Scheduler();
    void start();   // Begin evaluating and firing due tasks
    void stop();    // Stop the scheduler thread and clear running state
    void pause();   // Temporarily halt evaluation without tearing down the thread
    void resume();  // Resume after a pause
    void restart(); // Convenience: stop then start
    void setIntentRecognition(std::shared_ptr<I_IntentRecognition> intentRecognition);
    void setIntentRegistry(std::shared_ptr<IntentRegistry> intentRegistry);
    // CRUD on tasks (thread-safe)
    uint32_t addTask(const ScheduledTask& task);                // Returns handle
    void removeTask(const std::shared_ptr<Intent>& intent);     // Remove by pointer match
    void removeTask(const std::string& intentName);             // Remove by intent name
    void removeTask(uint32_t taskHandle);                       // Remove by handle

    // API Interface
    HttpEndPointData_t getHttpEndpointData() override; // Expose control + CRUD endpoints
    std::string getInterfaceName() const override;     // For API discovery

private:
    std::shared_ptr<I_IntentRecognition> intentRecognition;
    std::weak_ptr<IntentRegistry> intentRegistry;
    ScheduledTaskList scheduledTasks;
    std::jthread* schedulerThread;
    std::mutex schedulerMutex;
    std::condition_variable schedulerCV;
    bool schedulerRunning = false;
    bool schedulerPaused = false;
    void schedulerThreadFunction(std::stop_token st);  // Main loop; evaluates due tasks and sleeps
};

}
