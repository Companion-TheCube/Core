/*
████████╗██████╗ ██╗ ██████╗  ██████╗ ███████╗██████╗ ███████╗    ██████╗██████╗ ██████╗
╚══██╔══╝██╔══██╗██║██╔════╝ ██╔════╝ ██╔════╝██╔══██╗██╔════╝   ██╔════╝██╔══██╗██╔══██╗
   ██║   ██████╔╝██║██║  ███╗██║  ███╗█████╗  ██████╔╝███████╗   ██║     ██████╔╝██████╔╝
   ██║   ██╔══██╗██║██║   ██║██║   ██║██╔══╝  ██╔══██╗╚════██║   ██║     ██╔═══╝ ██╔═══╝
   ██║   ██║  ██║██║╚██████╔╝╚██████╔╝███████╗██║  ██║███████║██╗╚██████╗██║     ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝ ╚═════╝  ╚═════╝ ╚══════╝╚═╝  ╚═╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

// Triggers implementation: time- and event-based fireables plus a manager
// that polls and exposes HTTP control to create, enable, fire, and list.
#include "triggers.h"

namespace DecisionEngine {

TriggerManager::TriggerHandle TriggerManager::nextHandle = 0;

// I_Trigger base helpers
bool I_Trigger::isEnabled() const { return enabled; }
void I_Trigger::setEnabled(bool en) { enabled = en; }
bool I_Trigger::getTriggerState() const { return triggerState; }
void I_Trigger::setTriggerFunction(std::function<void()> fn) { triggerFunction = std::move(fn); }
void I_Trigger::setCheckTrigger(std::function<bool()> fn) { checkTrigger = std::move(fn); }
bool I_Trigger::hasCheck() const { return (bool)checkTrigger; }
bool I_Trigger::evaluateCheck() const { return checkTrigger ? checkTrigger() : false; }

////////////////////////////////////////////////////////////////////////////////////////////////////
// TimeTrigger

TimeTrigger::TimeTrigger()
{
    // default: never fires unless explicitly set and checked
    setCheckTrigger([this]() {
        return std::chrono::system_clock::now() >= time;
    });
}

TimeTrigger::TimeTrigger(const TimePoint& t)
    : TimeTrigger()
{
    time = t;
}

TimeTrigger::TimeTrigger(const TimePoint& t, const std::function<void()>& fn)
    : TimeTrigger(t)
{
    setTriggerFunction(fn);
}

TimeTrigger::TimeTrigger(const TimePoint& t, const std::function<void()>& fn, const std::function<bool()>& chk)
    : TimeTrigger(t, fn)
{
    setCheckTrigger(chk);
}

void TimeTrigger::trigger()
{
    if (!isEnabled())
        return;
    // If no checkTrigger provided, assume time-based check
    bool ok = true;
    if (auto ct = this->checkTrigger)
        ok = ct();
    if (ok) {
        triggerState = true;
        if (triggerFunction)
            triggerFunction();
    }
}

void TimeTrigger::setTime(const TimePoint& t)
{
    time = t;
}

const TimePoint& TimeTrigger::getTime() const { return time; }

void TimeTrigger::setScheduler(std::shared_ptr<Scheduler> s)
{
    scheduler = s;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EventTrigger

EventTrigger::EventTrigger() { }

EventTrigger::EventTrigger(const std::function<void()>& fn)
{
    setTriggerFunction(fn);
}

EventTrigger::EventTrigger(const std::function<void()>& fn, const std::function<bool()>& chk)
{
    setTriggerFunction(fn);
    setCheckTrigger(chk);
}

void EventTrigger::trigger()
{
    if (!isEnabled())
        return;
    bool ok = true;
    if (auto ct = this->checkTrigger)
        ok = ct();
    if (ok) {
        triggerState = true;
        if (triggerFunction)
            triggerFunction();
    }
}

void EventTrigger::setScheduler(std::shared_ptr<Scheduler> s)
{
    scheduler = s;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TriggerManager

TriggerManager::TriggerManager() { }

TriggerManager::TriggerManager(const std::shared_ptr<Scheduler>& s)
{
    setScheduler(s);
    pollThread = new std::jthread([this](std::stop_token st) {
        while (!st.stop_requested()) {
            {
                std::scoped_lock lk(mtx);
                for (auto& kv : triggers) {
                    auto& trig = kv.second;
                    if (trig && trig->isEnabled()) {
                        bool ok = trig->evaluateCheck();
                        if (ok && trig->hasCheck()) {
                            trig->trigger();
                            if (dynamic_cast<TimeTrigger*>(trig.get()))
                                trig->setEnabled(false);
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

TriggerManager::~TriggerManager()
{
    if (pollThread) {
        delete pollThread;
        pollThread = nullptr;
    }
}

void TriggerManager::setScheduler(std::shared_ptr<Scheduler> s)

{
    scheduler = std::move(s);
}

void TriggerManager::setIntentRegistry(std::shared_ptr<IntentRegistry> reg)
{
    intentRegistry = reg;
}

void TriggerManager::setFunctionRegistry(std::shared_ptr<FunctionRegistry> registry)
{
    this->functionRegistry = std::move(registry);
}

void TriggerManager::runFunctionAsync(const std::string& functionName,
    const nlohmann::json& args,
    std::function<void(const nlohmann::json&)> onComplete)
{
    if (!functionRegistry) {
        CubeLog::error("TriggerManager: FunctionRegistry not available when trying to run function: " + functionName);
        if (onComplete)
            onComplete(nlohmann::json({ { "error", "function_registry_unavailable" } }));
        return;
    }
    try {
        functionRegistry->runFunctionAsync(functionName, args, onComplete);
    } catch (const std::exception& e) {
        CubeLog::error(std::string("TriggerManager::runFunctionAsync exception: ") + e.what());
        if (onComplete)
            onComplete(nlohmann::json({ { "error", std::string("exception: ") + e.what() } }));
    }
}

void TriggerManager::runCapabilityAsync(const std::string& capabilityName,
    const nlohmann::json& args,
    std::function<void(const nlohmann::json&)> onComplete)
{
    if (!functionRegistry) {
        CubeLog::error("TriggerManager: FunctionRegistry not available when trying to run capability: " + capabilityName);
        if (onComplete)
            onComplete(nlohmann::json({ { "error", "function_registry_unavailable" } }));
        return;
    }
    try {
        functionRegistry->runCapabilityAsync(capabilityName, args, onComplete);
    } catch (const std::exception& e) {
        CubeLog::error(std::string("TriggerManager::runCapabilityAsync exception: ") + e.what());
        if (onComplete)
            onComplete(nlohmann::json({ { "error", std::string("exception: ") + e.what() } }));
    }
}

HttpEndPointData_t TriggerManager::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // POST /createTimeTrigger: Create a one-shot time trigger.
    // Body (application/json): { timeEpochMs?: number, delayMs?: number, intentName?: string }
    // - timeEpochMs (absolute) wins over delayMs (relative). If intentName is provided and found,
    //   the trigger will execute that intent when it fires; otherwise only sets triggerState.
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            // Validate content type and parse body; compute the trigger time.
            if (!(req.has_header("Content-Type") && req.get_header_value("Content-Type") == "application/json")) {
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Content-Type must be application/json";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Content-Type must be application/json");
            }
            try {
                auto j = nlohmann::json::parse(req.body);
                auto epochMs = j.value("timeEpochMs", (int64_t)0);
                auto delayMs = j.value("delayMs", (int64_t)0);
                auto intentName = j.value("intentName", std::string(""));
                auto capabilityName = j.value("capabilityName", std::string(""));
                auto functionName = j.value("functionName", std::string(""));
                nlohmann::json args = nlohmann::json::object();
                if (j.contains("args") && j["args"].is_object())
                    args = j["args"];
                TimePoint tp;
                if (epochMs > 0)
                    tp = TimePoint(std::chrono::milliseconds(epochMs));
                else
                    tp = std::chrono::system_clock::now() + std::chrono::milliseconds(delayMs);
                // Instantiate and enable the trigger; optionally bind to an intent.
                auto trig = std::make_shared<TimeTrigger>(tp);
                trig->setEnabled(true);
                trig->setScheduler(scheduler);
                if (!intentName.empty()) {
                    if (auto reg = intentRegistry.lock()) {
                        if (auto intent = reg->getIntent(intentName)) {
                            trig->setTriggerFunction([intent]() { intent->execute(); });
                        }
                    }
                } else if (!capabilityName.empty()) {
                    trig->setTriggerFunction([this, capabilityName, args]() {
                        this->runCapabilityAsync(capabilityName, args, nullptr);
                    });
                } else if (!functionName.empty()) {
                    trig->setTriggerFunction([this, functionName, args]() {
                        this->runFunctionAsync(functionName, args, nullptr);
                    });
                }
                // Store trigger under a unique handle; return it to the client.
                std::scoped_lock lk(mtx);
                TriggerHandle h = ++nextHandle;
                triggers[h] = trig;
                nlohmann::json out;
                out["success"] = true;
                out["handle"] = h;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (std::exception& e) {
                nlohmann::json out;
                out["success"] = false;
                out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "createTimeTrigger",
        nlohmann::json({ { "type", "object" },
            { "properties", { { "timeEpochMs", { { "type", "integer" } } }, { "delayMs", { { "type", "integer" } } }, { "intentName", { { "type", "string" } } }, { "capabilityName", { { "type", "string" } } }, { "functionName", { { "type", "string" } } }, { "args", { { "type", "object" } } } } },
            { "oneOf",
                nlohmann::json::array({ nlohmann::json::object({ { "required",
                                            nlohmann::json::array({ "timeEpochMs" }) } }),
                    nlohmann::json::object({ { "required",
                        nlohmann::json::array({ "delayMs" }) } }) }) } }),
        "Create a one-shot time trigger" });

    // POST /createEventTrigger: Create an event trigger with an optional bound intent.
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            // Validate content type; if intentName is provided and registered, bind it to the trigger action.
            if (!(req.has_header("Content-Type") && req.get_header_value("Content-Type") == "application/json")) {
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Content-Type must be application/json";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Content-Type must be application/json");
            }
            try {
                auto j = nlohmann::json::parse(req.body);
                auto intentName = j.value("intentName", std::string(""));
                auto capabilityName = j.value("capabilityName", std::string(""));
                auto functionName = j.value("functionName", std::string(""));
                nlohmann::json args = nlohmann::json::object();
                if (j.contains("args") && j["args"].is_object())
                    args = j["args"];

                auto trig = std::make_shared<EventTrigger>();
                trig->setEnabled(true);
                trig->setScheduler(scheduler);

                // Priority: explicit intentName -> capabilityName -> functionName
                if (!intentName.empty()) {
                    if (auto reg = intentRegistry.lock()) {
                        if (auto intent = reg->getIntent(intentName)) {
                            trig->setTriggerFunction([intent]() { intent->execute(); });
                        }
                    }
                } else if (!capabilityName.empty()) {
                    // Bind trigger to call a capability via FunctionRegistry
                    trig->setTriggerFunction([this, capabilityName, args]() {
                        this->runCapabilityAsync(capabilityName, args, nullptr);
                    });
                } else if (!functionName.empty()) {
                    // Bind trigger to call a function via FunctionRegistry
                    trig->setTriggerFunction([this, functionName, args]() {
                        this->runFunctionAsync(functionName, args, nullptr);
                    });
                }
                std::scoped_lock lk(mtx);
                TriggerHandle h = ++nextHandle;
                triggers[h] = trig;
                nlohmann::json out;
                out["success"] = true;
                out["handle"] = h;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (std::exception& e) {
                nlohmann::json out;
                out["success"] = false;
                out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "createEventTrigger",
        nlohmann::json({ { "type", "object" },
            { "properties", { { "intentName", { { "type", "string" } } }, { "capabilityName", { { "type", "string" } } }, { "functionName", { { "type", "string" } } }, { "args", { { "type", "object" } } } } },
            { "oneOf", nlohmann::json::array({ nlohmann::json::object({ { "required", nlohmann::json::array({ "intentName" }) } }), nlohmann::json::object({ { "required", nlohmann::json::array({ "capabilityName" }) } }), nlohmann::json::object({ { "required", nlohmann::json::array({ "functionName" }) } }) }) } }),
        "Create an event trigger" });

    // POST /setTriggerEnabled: Enable or disable an existing trigger by handle.
    // Body: { handle: number, enable: boolean }
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                auto handle = j.at("handle").get<TriggerHandle>();
                auto enable = j.value("enable", true);
                std::scoped_lock lk(mtx);
                if (!triggers.count(handle))
                    throw std::runtime_error("Trigger not found");
                triggers[handle]->setEnabled(enable);
                nlohmann::json out;
                out["success"] = true;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (std::exception& e) {
                nlohmann::json out;
                out["success"] = false;
                out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "setTriggerEnabled", { "handle", "enable" }, "Enable/disable a trigger" });

    // POST /fireTrigger: Manually fire a trigger by handle (useful for event-type triggers).
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                auto handle = j.at("handle").get<TriggerHandle>();
                std::shared_ptr<I_Trigger> trig;
                {
                    // Acquire trigger under lock, then release before invoking to avoid holding mutex during execution.
                    std::scoped_lock lk(mtx);
                    if (!triggers.count(handle))
                        throw std::runtime_error("Trigger not found");
                    trig = triggers[handle];
                }
                if (trig)
                    trig->trigger();
                nlohmann::json out;
                out["success"] = true;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (std::exception& e) {
                nlohmann::json out;
                out["success"] = false;
                out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "fireTrigger", { "handle" }, "Fire a trigger" });

    // POST /bindTrigger: Attach an action to an existing trigger by handle.
    // Body: { handle: number, intentName?: string, capabilityName?: string, functionName?: string, args?: object }
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                auto handle = j.at("handle").get<TriggerHandle>();
                std::string intentName = j.value("intentName", std::string(""));
                std::string capabilityName = j.value("capabilityName", std::string(""));
                std::string functionName = j.value("functionName", std::string(""));
                nlohmann::json args = nlohmann::json::object();
                if (j.contains("args") && j["args"].is_object())
                    args = j["args"];

                std::shared_ptr<I_Trigger> trig;
                {
                    std::scoped_lock lk(mtx);
                    if (!triggers.count(handle))
                        throw std::runtime_error("Trigger not found");
                    trig = triggers[handle];
                }

                if (!trig)
                    throw std::runtime_error("Trigger not found");

                // Attach according to priority: intentName -> capabilityName -> functionName
                if (!intentName.empty()) {
                    if (auto reg = intentRegistry.lock()) {
                        if (auto intent = reg->getIntent(intentName)) {
                            trig->setTriggerFunction([intent]() { intent->execute(); });
                        } else {
                            throw std::runtime_error("Intent not found: " + intentName);
                        }
                    } else {
                        throw std::runtime_error("IntentRegistry not available");
                    }
                } else if (!capabilityName.empty()) {
                    // Ensure function registry exists before binding
                    if (!functionRegistry)
                        throw std::runtime_error("FunctionRegistry not available");
                    trig->setTriggerFunction([this, capabilityName, args]() { this->runCapabilityAsync(capabilityName, args, nullptr); });
                } else if (!functionName.empty()) {
                    if (!functionRegistry)
                        throw std::runtime_error("FunctionRegistry not available");
                    trig->setTriggerFunction([this, functionName, args]() { this->runFunctionAsync(functionName, args, nullptr); });
                } else {
                    throw std::runtime_error("No binding provided");
                }

                nlohmann::json out;
                out["success"] = true;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            } catch (std::exception& e) {
                nlohmann::json out;
                out["success"] = false;
                out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "bindTrigger",
        nlohmann::json({ { "type", "object" },
            { "properties", { { "handle", { { "type", "integer" } } }, { "intentName", { { "type", "string" } } }, { "capabilityName", { { "type", "string" } } }, { "functionName", { { "type", "string" } } }, { "args", { { "type", "object" } } } } },
            { "oneOf", nlohmann::json::array({ nlohmann::json::object({ { "required", nlohmann::json::array({ "handle", "intentName" }) } }), nlohmann::json::object({ { "required", nlohmann::json::array({ "handle", "capabilityName" }) } }), nlohmann::json::object({ { "required", nlohmann::json::array({ "handle", "functionName" }) } }) }) } }),
        "Bind a trigger to an intent, capability, or function" });

    // GET /listTriggers: Return a summary of registered triggers.
    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            // Each entry includes handle, enabled flag, whether a predicate exists, and type (time/event).
            nlohmann::json j;
            j["success"] = true;
            j["triggers"] = nlohmann::json::array();
            std::scoped_lock lk(mtx);
            for (auto& kv : triggers) {
                nlohmann::json tj;
                tj["handle"] = kv.first;
                tj["enabled"] = kv.second->isEnabled();
                tj["hasCheck"] = (bool)kv.second->checkTrigger;
                tj["type"] = dynamic_cast<TimeTrigger*>(kv.second.get()) ? "time" : "event";
                j["triggers"].push_back(tj);
            }
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "listTriggers", {}, "List triggers" });
    return data;
}

constexpr std::string TriggerManager::getInterfaceName() const
{
    return "TriggerManager";
}

} // namespace DecisionEngine
