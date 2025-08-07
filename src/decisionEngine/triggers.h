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
#include "scheduler.h"

namespace DecisionEngine {

using TimePoint = std::chrono::system_clock::time_point;

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
    bool hasCheck() const;
    bool evaluateCheck() const;

private:
    bool enabled = false;
    bool schedulerSet = false;
protected:
    bool triggerState = false;
    std::function<void()> triggerFunction;
    std::function<bool()> checkTrigger;
    std::weak_ptr<Scheduler> scheduler;
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
    void setScheduler(std::shared_ptr<Scheduler> scheduler) override;

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
    void setScheduler(std::shared_ptr<Scheduler> scheduler) override;
};

/////////////////////////////////////////////////////////////////////////////////////

class TriggerManager : public AutoRegisterAPI<TriggerManager> {
public:
    TriggerManager();
    TriggerManager(const std::shared_ptr<Scheduler>& scheduler);
    ~TriggerManager();
    void setScheduler(std::shared_ptr<Scheduler> scheduler);
    void setIntentRegistry(std::shared_ptr<IntentRegistry> intentRegistry);
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;

private:
    std::shared_ptr<Scheduler> scheduler;
    std::weak_ptr<IntentRegistry> intentRegistry;
    using TriggerHandle = uint32_t;
    std::unordered_map<TriggerHandle, std::shared_ptr<I_Trigger>> triggers;
    std::jthread* pollThread{ nullptr };
    std::mutex mtx;
    static TriggerHandle nextHandle;
};

}
