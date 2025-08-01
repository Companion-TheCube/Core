/*
██████╗ ███████╗ ██████╗██╗███████╗██╗ ██████╗ ███╗   ██╗███████╗   ██╗  ██╗
██╔══██╗██╔════╝██╔════╝██║██╔════╝██║██╔═══██╗████╗  ██║██╔════╝   ██║  ██║
██║  ██║█████╗  ██║     ██║███████╗██║██║   ██║██╔██╗ ██║███████╗   ███████║
██║  ██║██╔══╝  ██║     ██║╚════██║██║██║   ██║██║╚██╗██║╚════██║   ██╔══██║
██████╔╝███████╗╚██████╗██║███████║██║╚██████╔╝██║ ╚████║███████║██╗██║  ██║
╚═════╝ ╚══════╝ ╚═════╝╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#ifndef DECISIONS_H
#define DECISIONS_H

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
#include "interfaceDefs.hpp"

#define LOCAL_INTENT_RECOGNITION_THREAD_COUNT 4
#define LOCAL_INTENT_RECOGNITION_THREAD_SLEEP_MS 100
#define SCHEDULER_THREAD_SLEEP_MS 100

namespace DecisionEngine {

using TimePoint = std::chrono::system_clock::time_point;


/////////////////////////////////////////////////////////////////////////////////////

class LocalTranscriber : public I_Transcriber {
public:
    LocalTranscriber();
    ~LocalTranscriber();
    std::string transcribeBuffer(const uint16_t* audio, size_t length) override;
    std::string transcribeStream(const uint16_t* audio, size_t bufSize) override;
    // TODO: the stream that this is reading from may need to be a more complex
    // datatype that has read and write pointers and a mutex to protect them.

private:
    std::shared_ptr<CubeWhisper> cubeWhisper;
};

/////////////////////////////////////////////////////////////////////////////////////

class RemoteTranscriber : public I_Transcriber, public I_RemoteApi {
public:
    RemoteTranscriber();
    ~RemoteTranscriber();
    std::string transcribeBuffer(const uint16_t* audio, size_t length) override;
    std::string transcribeStream(const uint16_t* audio, size_t bufSize) override;

private:
    bool initTranscribing();
    bool streamAudio();
    bool stopTranscribing();
};

/////////////////////////////////////////////////////////////////////////////////////

class ScheduledTask {
public:
    enum class RepeatInterval {
        REPEAT_NONE_ONE_SHOT,
        REPEAT_DAILY,
        REPEAT_WEEKLY,
        REPEAT_MONTHLY,
        REPEAT_YEARLY,
        REPEAT_CUSTOM
    };
    // A handle to a scheduled or schedule-able task
    using ScheduledTaskHandle = uint32_t;
    // The first element is the interval for the repeating task. The second element is the number of times the task should be repeated.
    using RepeatingType = std::pair<RepeatInterval, uint16_t>;
    struct ScheduleType {
        TimePoint time;
        RepeatingType repeat;
        TimePoint endTime;
    };

    ScheduledTask() = default;
    ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time);
    ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatingType& repeat);
    ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatingType& repeat, const TimePoint& endTime);
    const ScheduleType& getSchedule() const;
    const std::shared_ptr<Intent>& getIntent() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    ScheduledTaskHandle getHandle() const;
    void executeIntent();

private:
    ScheduleType schedule = { TimePoint(), { RepeatInterval::REPEAT_NONE_ONE_SHOT, 0 }, TimePoint() };
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
    constexpr std::string getInterfaceName() const override;

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

/////////////////////////////////////////////////////////////////////////////////////

class I_Trigger {
public:
    virtual ~I_Trigger() = default;
    virtual void trigger() = 0;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    bool getTriggerState() const;
    void setTriggerFunction(std::function<void()> triggerFunction);
    void setCheckTrigger(std::function<bool()> checkTrigger);
    virtual void setScheduler(std::shared_ptr<Scheduler> scheduler) = 0;

private:
    bool enabled = false;
    bool triggerState = false;
    bool schedulerSet = false;
    std::function<void()> triggerFunction;
    std::function<bool()> checkTrigger;
};

/////////////////////////////////////////////////////////////////////////////////////

class TimeTrigger : public I_Trigger {
public:
    TimeTrigger();
    TimeTrigger(const TimePoint& time);
    TimeTrigger(const TimePoint& time, const std::function<void()>& triggerFunction);
    TimeTrigger(const TimePoint& time, const std::function<void()>& triggerFunction, const std::function<bool()>& checkTrigger);
    void trigger() override;
    void setTime(const TimePoint& time);
    const TimePoint& getTime() const;

private:
    TimePoint time;
};

/////////////////////////////////////////////////////////////////////////////////////

class EventTrigger : public I_Trigger {
public:
    EventTrigger();
    EventTrigger(const std::function<void()>& triggerFunction);
    EventTrigger(const std::function<void()>& triggerFunction, const std::function<bool()>& checkTrigger);
    void trigger() override;
};

/////////////////////////////////////////////////////////////////////////////////////

class TriggerManager : public AutoRegisterAPI<TriggerManager> {
public:
    TriggerManager();
    void setScheduler(std::shared_ptr<Scheduler> scheduler);
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;

private:
    std::shared_ptr<Scheduler> scheduler;
};
/////////////////////////////////////////////////////////////////////////////////////

class DecisionEngineMain : public I_AudioQueue {
public:
    DecisionEngineMain();
    ~DecisionEngineMain();
    void start();
    void stop();
    void restart();
    void pause();
    void resume();

private:
    std::shared_ptr<I_IntentRecognition> intentRecognition;
    std::shared_ptr<IntentRegistry> intentRegistry;
    std::shared_ptr<I_Transcriber> transcriber;
    std::shared_ptr<Scheduler> scheduler;
    std::shared_ptr<TriggerManager> triggerManager;
    std::shared_ptr<Personality::PersonalityManager> personalityManager;
    std::shared_ptr<TheCubeServer::TheCubeServerAPI> remoteServerAPI;
};

/////////////////////////////////////////////////////////////////////////////////////

std::vector<IntentCTorParams> getSystemIntents();
std::vector<IntentCTorParams> getSystemSchedule();

std::future<std::string> modifyStringUsingAIForEmotionalState(const std::string& input, const std::vector<Personality::EmotionSimple>& emotions, const std::shared_ptr<TheCubeServer::TheCubeServerAPI>& remoteServerAPI, const std::function<void(std::string)>& progressCB);

}; // namespace DecisionEngine

#endif // DECISIONS_H
