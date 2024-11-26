#ifndef DECISIONS_H
#define DECISIONS_H

#include "../database/cubeDB.h"
#include "utils.h"
#include <logger.h>
#ifndef API_I_H
#include "../api/api_i.h"
#endif
#include "cubeWhisper.h"
#include "nlohmann/json.hpp"
#include "remoteServer.h"
#include <chrono>
#include <condition_variable>
#include <functional>
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
#include "httplib.h"
#include "../lazyLoader.h"
#include "../api/autoRegister.h"

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
    const std::string getResponseString() const;
    void setResponseString(const std::string& responseString);
    void setBriefDesc(const std::string& briefDesc);

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
    std::string responseString;
    std::string serializedData;
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

class IntentRegistry : public I_API_Interface {
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

class I_RemoteApi {
public:
    using Server = TheCubeServer::TheCubeServerAPI;
    virtual ~I_RemoteApi() = default;
    bool resetServerConnection();
    bool ableToCommunicateWithRemoteServer();
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

class RemoteIntentRecognition : public I_IntentRecognition, protected I_RemoteApi {
public:
    RemoteIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~RemoteIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;

private:
    std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) override;
    std::jthread* recognitionThread;
    httplib::Client cli;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_Transcriber {
public:
    virtual ~I_Transcriber() = default;
    virtual std::string transcribeBuffer(const uint16_t* audio, size_t length) = 0;
    virtual std::string transcribeStream(const uint16_t* audio, size_t bufSize) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

class LocalTranscriber : public I_Transcriber {
public:
    LocalTranscriber();
    std::string transcribeBuffer(const uint16_t* audio, size_t length) override;
    std::string transcribeStream(const uint16_t* audio, size_t bufSize) override;

private:
    std::shared_ptr<Whisper> m_whisper;
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
    ScheduleType schedule = {TimePoint(), {RepeatInterval::REPEAT_NONE_ONE_SHOT, 0}, TimePoint()};
    std::shared_ptr<Intent> intent;
    bool enabled = false;
    static ScheduledTaskHandle nextHandle;
    ScheduledTaskHandle handle = 0;
    unsigned int repeatCount = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

class Scheduler : public I_API_Interface {
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

class I_Trigger: public I_API_Interface {
public:
    virtual ~I_Trigger() = default;
    virtual void trigger() = 0;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    bool getTriggerState() const;
    void setTriggerFunction(std::function<void()> triggerFunction);
    void setCheckTrigger(std::function<bool()> checkTrigger);
    virtual void setScheduler(std::shared_ptr<Scheduler> scheduler) = 0;
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override = 0;
    constexpr std::string getInterfaceName() const override = 0;
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
class TriggerManager : public I_API_Interface, public AutoRegisterAPI<TriggerManager> {
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

class DecisionEngineMain {
public:
    DecisionEngineMain();
    ~DecisionEngineMain();
    void start();
    void stop();
    void restart();
    void pause();
    void resume();
    void setIntentRecognition(std::shared_ptr<I_IntentRecognition> intentRecognition);
    void setTranscriber(std::shared_ptr<Whisper> transcriber);
    void setIntentRegistry(std::shared_ptr<IntentRegistry> intentRegistry);
    void setAPIKey(const std::string& apiKey);
    void setAPIURL(const std::string& apiURL);
    void setAPIPort(const std::string& apiPort);
    void setAPIPath(const std::string& apiPath);

    const std::shared_ptr<IntentRegistry> getIntentRegistry() const;
    const std::shared_ptr<Scheduler> getScheduler() const;

private:
    std::shared_ptr<I_IntentRecognition> intentRecognition;
    std::shared_ptr<Whisper> transcriber;
    std::shared_ptr<IntentRegistry> intentRegistry;
    // std::shared_ptr<Transcriber> transcriber;
    std::string apiKey;
    std::string apiURL;
    std::string apiPort;
    std::string apiPath;
};

/////////////////////////////////////////////////////////////////////////////////////

std::vector<IntentCTorParams> getSystemIntents();
std::vector<IntentCTorParams> getSystemSchedule();

}; // namespace DecisionEngine

#endif // DECISIONS_H
