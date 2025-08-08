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

TimeTrigger::TimeTrigger(const TimePoint& t) : TimeTrigger()
{
    time = t;
}

TimeTrigger::TimeTrigger(const TimePoint& t, const std::function<void()>& fn) : TimeTrigger(t)
{
    setTriggerFunction(fn);
}

TimeTrigger::TimeTrigger(const TimePoint& t, const std::function<void()>& fn, const std::function<bool()>& chk) : TimeTrigger(t, fn)
{
    setCheckTrigger(chk);
}

void TimeTrigger::trigger()
{
    if (!isEnabled()) return;
    // If no checkTrigger provided, assume time-based check
    bool ok = true;
    if (auto ct = this->checkTrigger) ok = ct();
    if (ok) {
        triggerState = true;
        if (triggerFunction) triggerFunction();
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

EventTrigger::EventTrigger() {}

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
    if (!isEnabled()) return;
    bool ok = true;
    if (auto ct = this->checkTrigger) ok = ct();
    if (ok) {
        triggerState = true;
        if (triggerFunction) triggerFunction();
    }
}

void EventTrigger::setScheduler(std::shared_ptr<Scheduler> s)
{
    scheduler = s;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TriggerManager

TriggerManager::TriggerManager() {}

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

TriggerManager::~TriggerManager() {
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

HttpEndPointData_t TriggerManager::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // Create a time trigger
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!(req.has_header("Content-Type") && req.get_header_value("Content-Type") == "application/json")) {
                nlohmann::json j; j["success"] = false; j["message"] = "Content-Type must be application/json";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Content-Type must be application/json");
            }
            try {
                auto j = nlohmann::json::parse(req.body);
                auto epochMs = j.value("timeEpochMs", (int64_t)0);
                auto delayMs = j.value("delayMs", (int64_t)0);
                auto intentName = j.value("intentName", std::string(""));
                TimePoint tp;
                if (epochMs > 0) tp = TimePoint(std::chrono::milliseconds(epochMs));
                else tp = std::chrono::system_clock::now() + std::chrono::milliseconds(delayMs);
                auto trig = std::make_shared<TimeTrigger>(tp);
                trig->setEnabled(true);
                trig->setScheduler(scheduler);
                if (!intentName.empty()) {
                    if (auto reg = intentRegistry.lock()) {
                        if (auto intent = reg->getIntent(intentName)) {
                            trig->setTriggerFunction([intent]() { intent->execute(); });
                        }
                    }
                }
                std::scoped_lock lk(mtx);
                TriggerHandle h = ++nextHandle;
                triggers[h] = trig;
                nlohmann::json out; out["success"] = true; out["handle"] = h;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Time trigger created");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "createTimeTrigger", { "timeEpochMs|delayMs", "intentName?" }, "Create a one-shot time trigger" });

    // Create an event trigger
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!(req.has_header("Content-Type") && req.get_header_value("Content-Type") == "application/json")) {
                nlohmann::json j; j["success"] = false; j["message"] = "Content-Type must be application/json";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Content-Type must be application/json");
            }
            try {
                auto j = nlohmann::json::parse(req.body);
                auto intentName = j.value("intentName", std::string(""));
                auto trig = std::make_shared<EventTrigger>();
                trig->setEnabled(true);
                trig->setScheduler(scheduler);
                if (!intentName.empty()) {
                    if (auto reg = intentRegistry.lock()) {
                        if (auto intent = reg->getIntent(intentName)) {
                            trig->setTriggerFunction([intent]() { intent->execute(); });
                        }
                    }
                }
                std::scoped_lock lk(mtx);
                TriggerHandle h = ++nextHandle;
                triggers[h] = trig;
                nlohmann::json out; out["success"] = true; out["handle"] = h;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Event trigger created");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "createEventTrigger", { "intentName?" }, "Create an event trigger" });

    // Enable/disable trigger
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                auto handle = j.at("handle").get<TriggerHandle>();
                auto enable = j.value("enable", true);
                std::scoped_lock lk(mtx);
                if (!triggers.count(handle)) throw std::runtime_error("Trigger not found");
                triggers[handle]->setEnabled(enable);
                nlohmann::json out; out["success"] = true;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Trigger state updated");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "setTriggerEnabled", { "handle", "enable" }, "Enable/disable a trigger" });

    // Fire trigger (useful for event triggers)
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto j = nlohmann::json::parse(req.body);
                auto handle = j.at("handle").get<TriggerHandle>();
                std::shared_ptr<I_Trigger> trig;
                {
                    std::scoped_lock lk(mtx);
                    if (!triggers.count(handle)) throw std::runtime_error("Trigger not found");
                    trig = triggers[handle];
                }
                if (trig) trig->trigger();
                nlohmann::json out; out["success"] = true;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Trigger fired");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "fireTrigger", { "handle" }, "Fire a trigger" });

    // List triggers
    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            nlohmann::json j; j["success"] = true; j["triggers"] = nlohmann::json::array();
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
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Listed triggers");
        },
        "listTriggers", {}, "List triggers" });
    return data;
}

constexpr std::string TriggerManager::getInterfaceName() const
{
    return "TriggerManager";
}

} // namespace DecisionEngine
