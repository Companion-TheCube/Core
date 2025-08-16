/*
████████╗██████╗ ██╗ ██████╗  ██████╗ ███████╗██████╗ ███████╗   ██╗  ██╗
╚══██╔══╝██╔══██╗██║██╔════╝ ██╔════╝ ██╔════╝██╔══██╗██╔════╝   ██║  ██║
   ██║   ██████╔╝██║██║  ███╗██║  ███╗█████╗  ██████╔╝███████╗   ███████║
   ██║   ██╔══██╗██║██║   ██║██║   ██║██╔══╝  ██╔══██╗╚════██║   ██╔══██║
   ██║   ██║  ██║██║╚██████╔╝╚██████╔╝███████╗██║  ██║███████║██╗██║  ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝ ╚═════╝  ╚═════╝ ╚══════╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═╝
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


// Triggers: event/time-based signals that can schedule or directly execute intents
//
// Concepts
// - I_Trigger encapsulates the ability to decide “should I fire now?” (via checkTrigger)
//   and “what happens when I fire?” (via triggerFunction). It can optionally reference
//   a Scheduler to enqueue work rather than execute immediately.
// - TimeTrigger evaluates against a specific TimePoint (or custom check).
// - EventTrigger fires based on arbitrary external state exposed through checkTrigger.
// - TriggerManager owns and polls triggers, and exposes HTTP endpoints to create/manage them.
//
// Threading model
// - TriggerManager runs a polling std::jthread that evaluates each trigger’s check function
//   and invokes trigger() when conditions pass. Access is guarded by an internal mutex.
//
// Typical flow
// - Create trigger (time or event), optionally bind to an Intent via IntentRegistry
// - Enable trigger so the poll loop can consider it
// - On pass, trigger() sets internal state and invokes triggerFunction (or schedules work)
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
    // Base virtual interface for all triggers. A trigger is enabled/disabled and
    // may expose a check function (predicate). trigger() executes the bound action
    // if enabled and if the predicate returns true (or no predicate is set).
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
    std::function<bool()> checkTrigger;

private:
    bool enabled = false;
    bool schedulerSet = false;
protected:
    bool triggerState = false;
    std::function<void()> triggerFunction; // Executed when trigger() passes
    std::weak_ptr<Scheduler> scheduler;
};

/////////////////////////////////////////////////////////////////////////////////////

class TimeTrigger : public I_Trigger {
public:
    // Fires when current time >= time (unless a custom check is provided).
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
    // Fires when an arbitrary predicate evaluates true. Useful for sensor/UI events.
    EventTrigger();
    EventTrigger(const std::function<void()>& triggerFunction);
    EventTrigger(const std::function<void()>& triggerFunction, const std::function<bool()>& checkTrigger);
    void trigger() override;
    void setScheduler(std::shared_ptr<Scheduler> scheduler) override;
};

/////////////////////////////////////////////////////////////////////////////////////

class TriggerManager : public AutoRegisterAPI<TriggerManager> {
public:
    // Owns a set of triggers and polls them periodically. Exposes endpoints to
    // create time/event triggers, enable/disable, fire, and list.
    TriggerManager();
    TriggerManager(const std::shared_ptr<Scheduler>& scheduler);
    ~TriggerManager();
    void setScheduler(std::shared_ptr<Scheduler> scheduler);
    void setIntentRegistry(std::shared_ptr<IntentRegistry> intentRegistry);
    void setFunctionRegistry(std::shared_ptr<FunctionRegistry> registry);
    // Helpers: invoke functions/capabilities via the FunctionRegistry
    void runFunctionAsync(const std::string& functionName,
        const nlohmann::json& args,
        std::function<void(const nlohmann::json&)> onComplete = nullptr);

    void runCapabilityAsync(const std::string& capabilityName,
        const nlohmann::json& args,
        std::function<void(const nlohmann::json&)> onComplete = nullptr);
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;

private:
    std::shared_ptr<Scheduler> scheduler;
    std::weak_ptr<IntentRegistry> intentRegistry;
    std::shared_ptr<FunctionRegistry> functionRegistry;
    using TriggerHandle = uint32_t;
    std::unordered_map<TriggerHandle, std::shared_ptr<I_Trigger>> triggers;
    std::jthread* pollThread{ nullptr };
    std::mutex mtx;
    static TriggerHandle nextHandle;
};

}
