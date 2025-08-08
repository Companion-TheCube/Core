/*
███████╗ ██████╗██╗  ██╗███████╗██████╗ ██╗   ██╗██╗     ███████╗██████╗     ██████╗██████╗ ██████╗ 
██╔════╝██╔════╝██║  ██║██╔════╝██╔══██╗██║   ██║██║     ██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
███████╗██║     ███████║█████╗  ██║  ██║██║   ██║██║     █████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
╚════██║██║     ██╔══██║██╔══╝  ██║  ██║██║   ██║██║     ██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
███████║╚██████╗██║  ██║███████╗██████╔╝╚██████╔╝███████╗███████╗██║  ██║██╗╚██████╗██║     ██║     
╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚═════╝  ╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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


// Scheduler implementation: maintains a list of scheduled tasks and executes
// their intents when due. Exposes HTTP endpoints for control and CRUD.
#include "scheduler.h"

namespace DecisionEngine {

std::vector<IntentCTorParams> getSystemSchedule()
{
    std::vector<IntentCTorParams> intents;
    // Test intent
    IntentCTorParams testIntent;
    testIntent.intentName = "Test Schedule";
    testIntent.action = [](const Parameters& params, Intent intent) {
        CubeLog::info("Test schedule executed");
    };
    testIntent.briefDesc = "Test schedule description";
    testIntent.responseString = "Test schedule response";
    testIntent.type = Intent::IntentType::COMMAND;
    intents.push_back(testIntent);
    return intents;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO:
// Scheduler - class that schedules intents. This class will be responsible for storing triggers and references to the intents that
// they trigger. It will also be responsible for monitoring the states of the triggers and triggering the intents when the triggers are activated.
// This class will need to have a thread that runs in the background to monitor the triggers.
// This class will also need to implement the API interface so that other apps can interact with it.

ScheduledTask::ScheduledTaskHandle ScheduledTask::nextHandle = 1;

ScheduledTask::ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time)
{
    this->intent = intent;
    this->schedule.time = time;
    this->schedule.repeat = { RepeatInterval::Interval::REPEAT_NONE_ONE_SHOT, 0 };
    this->schedule.endTime = TimePoint();
    this->handle = nextHandle++;
    this->enabled = true;
}

ScheduledTask::ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatInterval& repeat)
{
    this->intent = intent;
    this->schedule.time = time;
    this->schedule.repeat = repeat;
    this->schedule.endTime = TimePoint();
    this->handle = nextHandle++;
    this->enabled = true;
}

ScheduledTask::ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatInterval& repeat, const TimePoint& endTime)
{
    this->intent = intent;
    this->schedule.time = time;
    this->schedule.repeat = repeat;
    this->schedule.endTime = endTime;
    this->handle = nextHandle++;
    this->enabled = true;
}

ScheduledTask::ScheduledTaskHandle ScheduledTask::getHandle() const
{
    return handle;
}

const ScheduledTask::ScheduleType& ScheduledTask::getSchedule() const
{
    return schedule;
}

const std::shared_ptr<Intent>& ScheduledTask::getIntent() const
{
    return intent;
}

bool ScheduledTask::isEnabled() const
{
    return enabled;
}

void ScheduledTask::setEnabled(bool enabled)
{
    this->enabled = enabled;
}

void ScheduledTask::executeIntent()
{
    intent->execute();
    repeatCount++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Start a background thread immediately; it idles until start() is called.
Scheduler::Scheduler()
{
    schedulerThread = new std::jthread([this](std::stop_token st) {
        schedulerThreadFunction(st);
    });
}

Scheduler::~Scheduler()
{
    delete schedulerThread;
}

void Scheduler::start()
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    schedulerRunning = true;
    schedulerCV.notify_all();
}

void Scheduler::stop()
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    schedulerRunning = false;
    schedulerCV.notify_all();
}

void Scheduler::pause()
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    schedulerPaused = true;
}

void Scheduler::resume()
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    schedulerPaused = false;
    schedulerCV.notify_all();
}

void Scheduler::restart()
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    schedulerRunning = false;
    schedulerPaused = false;
    schedulerCV.notify_all();
}

void Scheduler::setIntentRecognition(std::shared_ptr<I_IntentRecognition> intentRecognition)
{
    this->intentRecognition = intentRecognition;
}

void Scheduler::setIntentRegistry(std::shared_ptr<IntentRegistry> intentRegistry)
{
    this->intentRegistry = intentRegistry;
}

uint32_t Scheduler::addTask(const ScheduledTask& task)
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    scheduledTasks.push_back(task);
    return task.getHandle();
}

void Scheduler::removeTask(const std::shared_ptr<Intent>& intent)
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    for (auto it = scheduledTasks.begin(); it != scheduledTasks.end(); it++) {
        if (it->getIntent() == intent) {
            scheduledTasks.erase(it);
            return;
        }
    }
}

void Scheduler::removeTask(const std::string& intentName)
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    for (auto it = scheduledTasks.begin(); it != scheduledTasks.end(); it++) {
        if (it->getIntent()->getIntentName() == intentName) {
            scheduledTasks.erase(it);
            return;
        }
    }
}

void Scheduler::removeTask(uint32_t taskHandle)
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    for (auto it = scheduledTasks.begin(); it != scheduledTasks.end(); it++) {
        if (it->getHandle() == taskHandle) {
            scheduledTasks.erase(it);
            return;
        }
    }
}

// Thread loop: waits for start(), then cycles until paused/stopped, executing
// any due tasks. Repeats are not yet re-enqueued (TODO noted below).
void Scheduler::schedulerThreadFunction(std::stop_token st)
{
    std::unique_lock<std::mutex> lock(schedulerMutex);
    while (!st.stop_requested()) {
        while (!schedulerRunning) {
            schedulerCV.wait(lock);
        }
        while (!schedulerPaused) {
            for (auto& task : scheduledTasks) {
                if (task.isEnabled() && task.getSchedule().time <= std::chrono::system_clock::now()) {
                    task.executeIntent();
                    // TODO: check if the task should be repeated and if so, add it back to the scheduledTasks list
                }
            }
            schedulerCV.wait_until(lock, std::chrono::system_clock::now() + std::chrono::milliseconds(SCHEDULER_THREAD_SLEEP_MS));
        }
    }
}

// Define REST endpoints for runtime control and inspection:
// - start/stop/pause/resume
// - listTasks: enumerate current scheduled tasks with basic metadata
// - addTask/removeTask: manage tasks by handle
HttpEndPointData_t Scheduler::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // Control endpoints
    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->start();
            nlohmann::json j; j["success"] = true; j["message"] = "Scheduler started";
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Scheduler started");
        },
        "start", {}, "Start scheduler" });

    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->stop();
            nlohmann::json j; j["success"] = true; j["message"] = "Scheduler stopped";
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Scheduler stopped");
        },
        "stop", {}, "Stop scheduler" });

    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->pause();
            nlohmann::json j; j["success"] = true; j["message"] = "Scheduler paused";
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Scheduler paused");
        },
        "pause", {}, "Pause scheduler" });

    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->resume();
            nlohmann::json j; j["success"] = true; j["message"] = "Scheduler resumed";
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Scheduler resumed");
        },
        "resume", {}, "Resume scheduler" });

    // List tasks
    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            std::scoped_lock lk(this->schedulerMutex);
            nlohmann::json j; j["success"] = true; j["tasks"] = nlohmann::json::array();
            for (auto& t : scheduledTasks) {
                nlohmann::json tj;
                auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(t.getSchedule().time);
                tj["handle"] = t.getHandle();
                tj["enabled"] = t.isEnabled();
                tj["timeEpochMs"] = tp.time_since_epoch().count();
                tj["repeatSeconds"] = t.getSchedule().repeat.toSeconds();
                if (auto in = t.getIntent()) tj["intentName"] = in->getIntentName();
                j["tasks"].push_back(tj);
            }
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Listed tasks");
        },
        "listTasks", {}, "List scheduled tasks" });

    // Add task (one-shot or repeating)
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
                auto epochMs = j.value("timeEpochMs", (int64_t)0);
                auto delayMs = j.value("delayMs", (int64_t)0);
                auto repeatSec = j.value("repeatSeconds", (uint32_t)0);
                if (intentName.empty()) throw std::runtime_error("intentName is required");
                auto reg = intentRegistry.lock();
                if (!reg) throw std::runtime_error("IntentRegistry not available");
                auto intent = reg->getIntent(intentName);
                if (!intent) throw std::runtime_error("Intent not found: " + intentName);
                TimePoint tp;
                if (epochMs > 0) {
                    tp = TimePoint(std::chrono::milliseconds(epochMs));
                } else {
                    tp = std::chrono::system_clock::now() + std::chrono::milliseconds(delayMs);
                }
                ScheduledTask task;
                if (repeatSec > 0) {
                    task = ScheduledTask(intent, tp, { ScheduledTask::RepeatInterval::Interval::REPEAT_CUSTOM, repeatSec });
                } else {
                    task = ScheduledTask(intent, tp);
                }
                auto handle = this->addTask(task);
                nlohmann::json out; out["success"] = true; out["handle"] = handle;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Task added");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "addTask", { "intentName", "timeEpochMs|delayMs", "repeatSeconds" }, "Add a scheduled task" });

    // Remove task by handle
    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!(req.has_header("Content-Type") && req.get_header_value("Content-Type") == "application/json")) {
                nlohmann::json j; j["success"] = false; j["message"] = "Content-Type must be application/json";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Content-Type must be application/json");
            }
            try {
                auto j = nlohmann::json::parse(req.body);
                auto handle = j.at("handle").get<uint32_t>();
                this->removeTask(handle);
                nlohmann::json out; out["success"] = true;
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Task removed");
            } catch (std::exception& e) {
                nlohmann::json out; out["success"] = false; out["message"] = e.what();
                res.set_content(out.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
            }
        },
        "removeTask", { "handle" }, "Remove a task by handle" });

    return data;
}

std::string Scheduler::getInterfaceName() const
{
    return "Scheduler";
}

}
