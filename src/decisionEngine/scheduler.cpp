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

HttpEndPointData_t Scheduler::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // TODO:
    // 1. Add a task

    return data;
}

std::string Scheduler::getInterfaceName() const
{
    return "Scheduler";
}

}