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

#define LOCAL_INTENT_RECOGNITION_THREAD_COUNT 4
#define LOCAL_INTENT_RECOGNITION_THREAD_SLEEP_MS 100
#define SCHEDULER_THREAD_SLEEP_MS 100

namespace DecisionEngine {
class Intent;
struct IntentCTorParams;

using Parameters = std::unordered_map<std::string, std::string>;
using Action = std::function<void(const Parameters&, Intent)>;
using TimePoint = std::chrono::system_clock::time_point;

enum class DecisionErrorType {
    ERROR_NONE,
    INVALID_PARAMS,
    INTERNAL_ERROR,
    NO_ACTION_SET,
    UNKNOWN_ERROR
};

/////////////////////////////////////////////////////////////////////////////////////

class DecisionEngineError : public std::runtime_error {
    static uint32_t errorCount;
    DecisionErrorType errorType;

public:
    DecisionEngineError(const std::string& message, DecisionErrorType errorType = DecisionErrorType::UNKNOWN_ERROR);
};

/////////////////////////////////////////////////////////////////////////////////////

class Intent {
public:
    // TODO: add a mutex so that the calling of execute can be thread safe
    enum class IntentType {
        QUESTION,
        COMMAND
    } type;

    /**
     * @brief Parameters for the intent
     * The parameters are key value pairs that are used to pass data to the action function.
     * The first string is the key and the second string is the value. The key can be used
     * in response string to insert the value. See responseString for more information.
     */

    Intent() {};
    Intent(const std::string& intentName, const Action& action);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString, Intent::IntentType type);
    Intent(IntentCTorParams params);

    void setPersonalityManager(std::shared_ptr<Personality::PersonalityManager> personalityManager);

    const std::string& getIntentName() const;
    const Parameters& getParameters() const;

    void setParameters(const Parameters& parameters);
    bool setParameter(const std::string& key, const std::string& value);
    void addParameter(const std::string& key, const std::string& value);

    void execute() const;

    const std::string serialize();
    static std::shared_ptr<Intent> deserialize(const std::string& serializedIntent);
    void setSerializedData(const std::string& serializedData);
    const std::string& getSerializedData() const;

    const std::string& getBriefDesc() const;
    void setBriefDesc(const std::string& briefDesc);
    const std::string getResponseString() const;
    void setResponseString(const std::string& responseString);
    void setScoredResponseStrings(const std::vector<std::string>& responseStrings);
    void setScoredResponseString(const std::string& responseString, int score);

    void setEmotionalScoreRanges(const std::vector<Personality::EmotionRange>& emotionRanges);
    void setEmotionScoreRange(const Personality::EmotionRange& emotionRange);

private:
    std::string intentName;
    Action action;
    Parameters parameters;
    std::string briefDesc;
    /**
     * @brief This is the string that will be returned to the user when the intent is executed. If TTS
     * is enabled, this string will be spoken to the user. This string can contain placeholders for the parameters.
     * The placeholders will be replaced with the actual values. The placeholders should be in the format ${parameterName}
     */
    std::string responseString = "";
    std::string serializedData = "";
    /**
     * @brief These strings should represent the response string at different emotional scores. The strings should be ordered from
     * closest to the emotional target to farthest from the emotional target. If the emotional score is out of range, the standard
     * response string will be used. Therefore, this vector can be 0 to 5 strings long, inclusive.
     */
    std::vector<std::string> responseStringScored = {};
    std::shared_ptr<Personality::PersonalityManager> personalityManager;

    std::vector<Personality::EmotionRange> emotionRanges;
};

/////////////////////////////////////////////////////////////////////////////////////

struct IntentCTorParams {
    IntentCTorParams() {};
    std::string intentName = "";
    Action action = nullptr;
    Parameters parameters = Parameters();
    std::string briefDesc = "";
    std::string responseString = "";
    Intent::IntentType type = Intent::IntentType::COMMAND;
};

/////////////////////////////////////////////////////////////////////////////////////

class IntentRegistry : public AutoRegisterAPI<IntentRegistry> {
public:
    IntentRegistry();
    bool registerIntent(const std::string& intentName, const std::shared_ptr<Intent> intent);
    bool unregisterIntent(const std::string& intentName);
    std::shared_ptr<Intent> getIntent(const std::string& intentName);
    std::vector<std::string> getIntentNames();
    std::vector<std::shared_ptr<Intent>> getRegisteredIntents();
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;

private:
    /**
     * @brief Map of intent names to intents
     */
    std::unordered_map<std::string, std::shared_ptr<Intent>> intentMap;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_AudioQueue {
public:
    virtual ~I_AudioQueue() = default;
    void setThreadSafeQueue(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
    {
        this->audioQueue = audioQueue;
    }
    const std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> getThreadSafeQueue()
    {
        return audioQueue;
    }

protected:
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_RemoteApi {
public:
    using Server = TheCubeServer::TheCubeServerAPI;
    virtual ~I_RemoteApi() = default;
    bool resetServerConnection();
    Server::ServerStatus getServerStatus();
    Server::ServerError getServerError();
    Server::ServerState getServerState();
    Server::FourBit getAvailableServices();
    void setRemoteServerAPIObject(std::shared_ptr<Server> remoteServerAPI);

protected:
    std::shared_ptr<Server> remoteServerAPI;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_IntentRecognition {
public:
    virtual ~I_IntentRecognition() = default;
    virtual std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) = 0;
    virtual bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) = 0;
    virtual bool recognizeIntentAsync(const std::string& intentString) = 0;
    std::shared_ptr<IntentRegistry> intentRegistry;
};

/////////////////////////////////////////////////////////////////////////////////////

class LocalIntentRecognition : public I_IntentRecognition {
public:
    LocalIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~LocalIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;

private:
    std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) override;
    std::vector<std::jthread*> recognitionThreads;
    std::vector<std::shared_ptr<TaskQueueWithData<std::function<void()>, std::string>>> taskQueues;
    bool threadsReady = false;
    // Pattern matching
    // Weighted pattern matching
    // Machine learning?
};

/////////////////////////////////////////////////////////////////////////////////////

class RemoteIntentRecognition : public I_IntentRecognition, public I_RemoteApi {
public:
    RemoteIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~RemoteIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;

private:
    std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) override;
    std::jthread* recognitionThread;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_Transcriber: public I_AudioQueue {
public:
    virtual ~I_Transcriber() = default;
    virtual std::string transcribeBuffer(const uint16_t* audio, size_t length) = 0;
    virtual std::string transcribeStream(const uint16_t* audio, size_t bufSize) = 0;
};

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
