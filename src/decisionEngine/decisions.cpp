/*
██████╗ ███████╗ ██████╗██╗███████╗██╗ ██████╗ ███╗   ██╗███████╗    ██████╗██████╗ ██████╗ 
██╔══██╗██╔════╝██╔════╝██║██╔════╝██║██╔═══██╗████╗  ██║██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██║  ██║█████╗  ██║     ██║███████╗██║██║   ██║██╔██╗ ██║███████╗   ██║     ██████╔╝██████╔╝
██║  ██║██╔══╝  ██║     ██║╚════██║██║██║   ██║██║╚██╗██║╚════██║   ██║     ██╔═══╝ ██╔═══╝ 
██████╔╝███████╗╚██████╗██║███████║██║╚██████╔╝██║ ╚████║███████║██╗╚██████╗██║     ██║     
╚═════╝ ╚══════╝ ╚═════╝╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

/*
              ████████╗██╗  ██╗███████╗
              ╚══██╔══╝██║  ██║██╔════╝
                 ██║   ███████║█████╗
                 ██║   ██╔══██║██╔══╝
                 ██║   ██║  ██║███████╗
                 ╚═╝   ╚═╝  ╚═╝╚══════╝
██████╗ ███████╗ ██████╗██╗███████╗██╗ ██████╗ ███╗   ██╗
██╔══██╗██╔════╝██╔════╝██║██╔════╝██║██╔═══██╗████╗  ██║
██║  ██║█████╗  ██║     ██║███████╗██║██║   ██║██╔██╗ ██║
██║  ██║██╔══╝  ██║     ██║╚════██║██║██║   ██║██║╚██╗██║
██████╔╝███████╗╚██████╗██║███████║██║╚██████╔╝██║ ╚████║
╚═════╝ ╚══════╝ ╚═════╝╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝
    ███████╗███╗   ██╗ ██████╗ ██╗███╗   ██╗███████╗
    ██╔════╝████╗  ██║██╔════╝ ██║████╗  ██║██╔════╝
    █████╗  ██╔██╗ ██║██║  ███╗██║██╔██╗ ██║█████╗
    ██╔══╝  ██║╚██╗██║██║   ██║██║██║╚██╗██║██╔══╝
    ███████╗██║ ╚████║╚██████╔╝██║██║ ╚████║███████╗
    ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝╚═╝  ╚═══╝╚══════╝

The decision engine will be responsible for making decisions based on user commands, schedules,
triggers, and the personality of TheCube.
It will provide API endpoints for other apps on the system to interact with it.
When the speech in class detects a wake word, it will route audio into the decision engine
which will then pass the audio to the TheCube Server service for translation to text. If the user
has not configured the system to use the remote server, we may use the whisper.cpp library
to do the translation locally.

Once the decision engine gets the text of the audio, it will then pass the text to the intent
detection service which will determine the intent of the user. The decision engine will then
use the response from the intent detection service to make a decision on what to do next.

To be evaluated: Using Whisper locally for users that don't want to pay for the service.
Whisper.cpp (https://github.com/ggerganov/whisper.cpp) will be the best option for this,
however it is not as accurate as the remote service and we'll have to implement our own intent detection.

*/

#include "decisions.h"

using namespace DecisionEngine;

// DecisionEngine - Main class that connects all the other classes together - this will need to connect to the personalityManager.
DecisionEngineMain::DecisionEngineMain()
{
    /*
    TODO:
    - Set up the connection to the speechIn class
    - Set up the connection to the TheCube Server API
    - Set up the connection to the personalityManager
    - Set up the scheduler
        - Scheduler needs to interface with the personalityManager so that certain things can be
        scheduled based on the personality of TheCube. For instance, if the playfulness setting
        is set sort of high, the scheduler might schedule some sort of animation or something to
        welcome the user back to their desk.

    The Process:
    - Once the wakeword is triggered, start moving audio from the speechIn class to the transcriber
    - If the user has remote transcription enabled:
        - The CubeServerAPI will then send the audio to the remote server for transcription
    - If the user does not have remote transcription enabled
        - The transcriber will use the whisper class to transcribe the audio
    - then we send the transcription to the intentRecognition class.
    - The intentRecognition class will then determine the intent of the user
        - if remote intent detection is enabled, intentRecognition will send the transcription
        to the remote server for intent detection
        - if remote intent detection is not enabled, intentRecognition will use the local intent detection
    - The intentRecognition class will then return the intent to the DecisionEngineMain class
    */

    //
    this->audioQueue = AudioManager::audioInQueue;
    remoteServerAPI = std::make_shared<TheCubeServer::TheCubeServerAPI>(audioQueue);
    intentRegistry = std::make_shared<IntentRegistry>();

    bool remoteIntentRecognition = GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::REMOTE_INTENT_RECOGNITION_ENABLED);
    if (remoteIntentRecognition) {
        intentRecognition = std::make_shared<RemoteIntentRecognition>(intentRegistry);
        (std::dynamic_pointer_cast<RemoteIntentRecognition>(intentRecognition))->setRemoteServerAPIObject(remoteServerAPI);
    } else
        intentRecognition = std::make_shared<LocalIntentRecognition>(intentRegistry);
    intentRecognition = std::make_shared<LocalIntentRecognition>(intentRegistry);
    bool remoteTranscription = GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::REMOTE_TRANSCRIPTION_ENABLED);
    if (remoteTranscription) {
        transcriber = std::make_shared<RemoteTranscriber>();
        (std::dynamic_pointer_cast<RemoteTranscriber>(transcriber))->setRemoteServerAPIObject(remoteServerAPI);
    } else
        transcriber = std::make_shared<LocalTranscriber>();



    ///////////// Testing /////////////
    // TODO: remove this test code
    auto pMan = std::make_shared<Personality::PersonalityManager>();
    pMan->registerInterface();
    std::vector<Personality::EmotionRange> inputRange = {
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::CURIOSITY },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::PLAYFULNESS },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::EMPATHY },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::ASSERTIVENESS },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::ATTENTIVENESS },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::CAUTION },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::ANNOYANCE }
    };
    auto score = pMan->calculateEmotionalMatchScore(inputRange);
    CubeLog::fatal("Emotional match score: " + std::to_string(score));

    
    for (size_t i = 0; i < 20; i++) {
        IntentCTorParams params;
        params.intentName = "Test Intent" + std::to_string(i);
        params.action = [i](const Parameters& params, Intent intent) {
            std::cout << "Test intent executed: " << std::to_string(i) << std::endl;
            intent.setParameter("TestParam", "--The new " + std::to_string(i) + " testValue--");
            CubeLog::fatal(intent.getResponseString());
        };
        params.parameters = Parameters({ { "TestParam", "testValue" } });
        params.briefDesc = "Test intent description: " + std::to_string(i);
        params.responseString = "Test intent response ${TestParam}";
        std::shared_ptr<Intent> testIntent = std::make_shared<Intent>(params);
        if (!intentRegistry->registerIntent("Test Intent: " + std::to_string(i), testIntent))
            CubeLog::error("Failed to register test intent" + std::to_string(i));
    }
    intentRecognition->recognizeIntentAsync("Do TestParam", [](std::shared_ptr<Intent> intent) {
        intent->execute();
    });
}

DecisionEngineMain::~DecisionEngineMain()
{
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO:
// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper class. Whisper class should be initialized and a reference to it saved for future use.
LocalTranscriber::LocalTranscriber()
{
    // TODO: this whole stupid thing
}
LocalTranscriber::~LocalTranscriber()
{
}
std::string LocalTranscriber::transcribeBuffer(const uint16_t* audio, size_t length)
{
    return "";
}
std::string LocalTranscriber::transcribeStream(const uint16_t* audio, size_t bufSize)
{
    return "";
}
// RemoteTranscriber - class that interacts with the TheCube Server API.
RemoteTranscriber::RemoteTranscriber()
{
    // TODO: also all of this one too
}
RemoteTranscriber::~RemoteTranscriber()
{
}
std::string RemoteTranscriber::transcribeBuffer(const uint16_t* audio, size_t length)
{
    return "";
}
std::string RemoteTranscriber::transcribeStream(const uint16_t* audio, size_t bufSize)
{
    return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interface: IntentRecognition - class that determines the intent of the user
// LocalIntentRecognition - class that determines the intent of the user without the need of an LLM. Use a score based pattern matching system.
LocalIntentRecognition::LocalIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry)
{
    this->intentRegistry = intentRegistry;
    // create the recognition threads
    for (size_t i = 0; i < LOCAL_INTENT_RECOGNITION_THREAD_COUNT; i++) {
        taskQueues.push_back(std::shared_ptr<TaskQueueWithData<std::function<void()>, std::string>>(new TaskQueueWithData<std::function<void()>, std::string>()));
        recognitionThreads.push_back(new std::jthread([this, i](std::stop_token st) {
            while (!st.stop_requested()) {
                genericSleep(LOCAL_INTENT_RECOGNITION_THREAD_SLEEP_MS);
                while (taskQueues[i]->size() != 0) {
                    auto task = taskQueues[i]->pop();
                    if (task) {
                        task();
                    }
                }
            }
        }));
    }
    threadsReady = true;
}

std::shared_ptr<Intent> LocalIntentRecognition::recognizeIntent(const std::string& name, const std::string& intentString)
{
    // TODO: although this code works, we need to implement a more advanced pattern matching system and we need to
    // have pattern matches that are weighted. For example, if the user says "What time is it?" we need to have a pattern
    // match for "What time is it?" and "What is the time?" and "What time is it now?" and "What is the time now?" and so on.
    // TODO: This function should somehow return a score for the match so that when multiple intents match, we can
    // choose the one with the highest score.

    auto l_intent = intentRegistry->getIntent(name);
    // first we make a vector of all the intents param names
    std::vector<std::vector<std::string>> intentParamNames;
    std::vector<std::string> paramNames;
    for (auto& param : l_intent->getParameters()) {
        paramNames.push_back(param.first);
    }
    intentParamNames.push_back(paramNames);
    // now we need to tokenize the intentString
    std::vector<std::string> tokens;
    std::regex splitOnRegex("[\\s,.]+");
    std::sregex_token_iterator iter(intentString.begin(), intentString.end(), splitOnRegex, -1);
    std::sregex_token_iterator end;
    while (iter != end) {
        tokens.push_back(iter->str());
        ++iter;
    }
    // now we need to find the intent that matches the most tokens
    size_t maxMatchIndex = 0;
    size_t maxMatchCount = 0;
    for (size_t i = 0; i < intentParamNames.size(); i++) {
        size_t matchCount = 0;
        for (auto& token : tokens) {
            for (auto& paramName : intentParamNames[i]) {
                if (token == paramName) {
                    matchCount++;
                    break;
                }
            }
        }
        if (matchCount > maxMatchCount) {
            maxMatchCount = matchCount;
            maxMatchIndex = i;
        }
    }
    if (maxMatchCount == 0)
        return nullptr;
    return l_intent;
}

bool LocalIntentRecognition::recognizeIntentAsync(const std::string& intentString)
{
    return this->recognizeIntentAsync(intentString, [](std::shared_ptr<Intent> intent) {});
}

// TODO: convert this to std::future and make callback the progress callback or remove it
bool LocalIntentRecognition::recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback)
{
    if (!threadsReady)
        return false;
    for (auto name : intentRegistry->getIntentNames()) {
        size_t minIndex = 0;
        size_t minSize = taskQueues[0]->size();
        for (size_t i = 1; i < taskQueues.size(); i++) {
            if (taskQueues[i]->size() < minSize) {
                minSize = taskQueues[i]->size();
                minIndex = i;
            }
        }
        taskQueues[minIndex]->push([this, intentString, callback, name]() {
            auto intent = recognizeIntent(name, intentString);
            if (intent)
                callback(intent);
        },
            intentString);
    }
    return true;
}

LocalIntentRecognition::~LocalIntentRecognition()
{
    for (auto& thread : recognitionThreads) {
        delete thread;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// RemoteIntentRecognition - class that converts intent to action using the TheCube Server API
RemoteIntentRecognition::RemoteIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry)
{
    this->intentRegistry = intentRegistry;
}

RemoteIntentRecognition::~RemoteIntentRecognition()
{
}

// TODO: convert this to std::future
bool RemoteIntentRecognition::recognizeIntentAsync(const std::string& intentString)
{
    return this->recognizeIntentAsync(intentString, [](std::shared_ptr<Intent> intent) {});
}

// TODO: convert this to std::future and make callback the progress callback or remove it
bool RemoteIntentRecognition::recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback)
{
    if (!remoteServerAPI)
        return false;
    return false; // TODO:
}

std::shared_ptr<Intent> RemoteIntentRecognition::recognizeIntent(const std::string& name, const std::string& intentString)
{
    if (!remoteServerAPI)
        return nullptr;
    return nullptr; // TODO:
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IntentRegistry - class that contains the intents that are registered with the system
// Need to have Http endpoints for the API that allow other apps to register with the decision engine
IntentRegistry::IntentRegistry()
{
    for (auto& intent : getSystemIntents()) {
        auto newIntent = std::make_shared<Intent>(intent);
        if (!registerIntent(intent.intentName, newIntent))
            CubeLog::error("Failed to register intent: " + intent.intentName);
    }

    // TODO: remove this. Testing only.
    httplib::Client cli("https://dummyjson.com");
    auto res = cli.Get("/test");
    if (res && res->status == 200) {
        auto text = res->body;
        CubeLog::fatal(text);
    } else {
        CubeLog::error("Error getting test data");
    }
}

bool IntentRegistry::registerIntent(const std::string& intentName, const std::shared_ptr<Intent> intent)
{
    // Check if the intent is already registered
    if (intentMap.find(intentName) != intentMap.end())
        return false;
    intentMap[intentName] = intent;
    return true;
}

bool IntentRegistry::unregisterIntent(const std::string& intentName)
{
    // Check if the intent is registered
    if (intentMap.find(intentName) == intentMap.end())
        return false;
    intentMap.erase(intentName);
    return true;
}

std::shared_ptr<Intent> IntentRegistry::getIntent(const std::string& intentName)
{
    if (intentMap.find(intentName) == intentMap.end())
        return nullptr;
    return intentMap[intentName];
}

std::vector<std::string> IntentRegistry::getIntentNames()
{
    std::vector<std::string> intentNames;
    for (auto& intent : intentMap)
        intentNames.push_back(intent.first);
    return intentNames;
}

std::vector<std::shared_ptr<Intent>> IntentRegistry::getRegisteredIntents()
{
    auto intents = std::vector<std::shared_ptr<Intent>>();
    for (auto& intent : intentMap)
        intents.push_back(intent.second);
    return intents;
}

HttpEndPointData_t IntentRegistry::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // TODO:
    // 1. Register an Intent - This will need to take a JSON string object that contains the intent data and pass it to the deserialize function of the Intent class.
    // 2. Unregister an Intent
    // 3. Get an Intent
    // 4. Get all Intent Names
    return data;
}

constexpr std::string IntentRegistry::getInterfaceName() const
{
    return "IntentRegistry";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get all the built-in system intents
 * @return std::vector<IntentCTorParams>
 */
std::vector<IntentCTorParams> DecisionEngine::getSystemIntents()
{
    std::vector<IntentCTorParams> intents;
    // Test intent
    IntentCTorParams testIntent;
    testIntent.intentName = "Test Intent";
    testIntent.action = [](const Parameters& params, Intent intent) {
        CubeLog::info("Test intent executed");
    };
    testIntent.briefDesc = "Test intent description";
    testIntent.responseString = "Test intent response";
    testIntent.type = Intent::IntentType::COMMAND;
    intents.push_back(testIntent);
    // TODO: define all the system intents. this should include things like "What time is it?" and "What apps are installed?"
    // Should also include some pure command intents like "Do a flip" or "What was that last notification?"
    return intents;
}

std::vector<IntentCTorParams> DecisionEngine::getSystemSchedule()
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
    this->schedule.repeat = { RepeatInterval::REPEAT_NONE_ONE_SHOT, 0 };
    this->schedule.endTime = TimePoint();
    this->handle = nextHandle++;
    this->enabled = true;
}

ScheduledTask::ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatingType& repeat)
{
    this->intent = intent;
    this->schedule.time = time;
    this->schedule.repeat = repeat;
    this->schedule.endTime = TimePoint();
    this->handle = nextHandle++;
    this->enabled = true;
}

ScheduledTask::ScheduledTask(const std::shared_ptr<Intent>& intent, const TimePoint& time, const RepeatingType& repeat, const TimePoint& endTime)
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

constexpr std::string Scheduler::getInterfaceName() const
{
    return "Scheduler";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO:
// Triggers need to work with the personalityManager so that they can be scheduled based on the personality of TheCube.
// Each trigger will need to have some definition of what personality settings it is looking for. For example, a trigger that
// plays a sound when the user returns to the desk might only be active if the playfulness setting is set to a certain level.
// Interface: Trigger - class that triggers intents. Stores a reference to whatever state it is monitoring.
// TimeBasedTrigger - class that triggers intents based on time. This will use the scheduler to schedule intents.
// EventBasedTrigger - class that triggers intents based on events like the user returning to the desk
// APITrigger - class that triggers intents based on API calls

bool I_Trigger::isEnabled() const
{
    return enabled;
}

void I_Trigger::setEnabled(bool enabled)
{
    this->enabled = enabled;
}

void I_Trigger::trigger()
{
    if (enabled)
        triggerFunction();
}

bool I_Trigger::getTriggerState() const
{
    return checkTrigger();
}

void I_Trigger::setTriggerFunction(std::function<void()> triggerFunction)
{
    this->triggerFunction = triggerFunction;
}

void I_Trigger::setCheckTrigger(std::function<bool()> checkTrigger)
{
    this->checkTrigger = checkTrigger;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool I_RemoteApi::resetServerConnection()
{
    return remoteServerAPI->resetServerConnection();
}

I_RemoteApi::Server::ServerStatus I_RemoteApi::getServerStatus()
{
    return remoteServerAPI->getServerStatus();
}

I_RemoteApi::Server::ServerError I_RemoteApi::getServerError()
{
    return remoteServerAPI->getServerError();
}

I_RemoteApi::Server::ServerState I_RemoteApi::getServerState()
{
    return remoteServerAPI->getServerState();
}

I_RemoteApi::Server::FourBit I_RemoteApi::getAvailableServices()
{
    return remoteServerAPI->services;
}

void I_RemoteApi::setRemoteServerAPIObject(std::shared_ptr<I_RemoteApi::Server> remoteServerAPI)
{
    this->remoteServerAPI = remoteServerAPI;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t DecisionEngineError::errorCount = 0;
DecisionEngineError::DecisionEngineError(const std::string& message, DecisionErrorType errorType)
    : std::runtime_error(message)
{
    this->errorType = errorType;
    CubeLog::error("DecisionEngineError: " + message);
    errorCount++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Modify a string to align with a given emotional state.
 *
 * @param input The string to modify
 * @param emotions The emotional state to align with
 * @param remoteServerAPI The remote server API object
 * @param progressCB The callback function to call with progress updates. Will be called on each of the response fragments from the LLM.
 * @return std::future<std::string> A future that will yield a JSON string with the modified string under the key "output". May contain other keys.
 */
std::future<std::string> DecisionEngine::modifyStringUsingAIForEmotionalState(const std::string& input, const std::vector<Personality::EmotionSimple>& emotions, const std::shared_ptr<TheCubeServer::TheCubeServerAPI>& remoteServerAPI, const std::function<void(std::string)>& progressCB)
{
    static const std::string chatString = "I have a robot program with an emotional state defined by 7 qualities: curiosity, playfulness, empathy, assertiveness, attentiveness, caution, and annoyance. Each quality has a value between 1 and 100 (inclusive). The robot interacts with the user by speaking sentences, and I need you to revise a given string to align with its emotional state.\n\n"
                                          "Here are the definitions of each quality:\n"
                                          "Curiosity (1-100): Reflects the robot’s interest in exploring or learning. High values make it more inquisitive or engaged.\n"
                                          "Playfulness (1-100): Reflects the robot's tendency to be lighthearted and fun. High values lead to a more cheerful and humorous tone.\n"
                                          "Empathy (1-100): Reflects the robot's capacity for understanding and kindness. High values result in more compassionate or sensitive language.\n"
                                          "Assertiveness (1-100): Reflects the robot's directness and determination. High values lead to more confident and action-oriented language.\n"
                                          "Attentiveness (1-100): Reflects the robot's focus and precision. High values make it more detailed and thorough.\n"
                                          "Caution (1-100): Reflects the robot's tendency to warn or hesitate. High values make it more reserved or cautious.\n"
                                          "Annoyance (1-100): Reflects the robot’s irritation level. High values lead to more curt or sarcastic language.\n\n"
                                          "When I provide a string, the robot's emotional state will also be provided as a list of 7 values, each corresponding to the qualities above. Modify the string to align with these values while keeping the robot’s response functional and understandable.\n\n"
                                          "Example:\n\n"
                                          "Input:\n"
                                          "String: \"You have eight items on your to-do list today.\"\n"
                                          "Emotional State: {Curiosity: 40, Playfulness: 20, Empathy: 50, Assertiveness: 90, Attentiveness: 80, Caution: 30, Annoyance: 10}\n\n"
                                          "Output:\n"
                                          "\"You have eight items on your to-do list that must be accomplished today.\"\n\n"
                                          "Another Example:\n\n"
                                          "Input:\n"
                                          "String: \"You have eight items on your to-do list today.\"\n"
                                          "Emotional State: {Curiosity: 70, Playfulness: 60, Empathy: 80, Assertiveness: 20, Attentiveness: 50, Caution: 40, Annoyance: 10}\n\n"
                                          "Output:\n"
                                          "\"You've got eight items on your to-do list today. Let me know if you'd like help organizing them!\"\n\n"
                                          "Request:\n"
                                          "Modify the following string to align with the given emotional state. Use the emotional state values to adjust tone, word choice, and phrasing appropriately while retaining the core message.  Your response should be a JSON object with the key \"output\" and a value that is only the revised string. Other keys are permissible.\n\n"
                                          "Input:\n";
    std::string inputString = "String: \"" + input + "\"\nEmotional State: {";
    for (size_t i = 0; i < emotions.size(); i++) {
        inputString += Personality::emotionToString(emotions[i].emotion) + ": " + std::to_string(emotions[i].value);
        if (i < emotions.size() - 1)
            inputString += ", ";
    }
    inputString += "}";
    return remoteServerAPI->getChatResponseAsync(chatString + inputString, progressCB);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <int N>
struct Fibonacci {
    static const int value = Fibonacci<N - 1>::value + Fibonacci<N - 2>::value;
};

template <>
struct Fibonacci<0> {
    static const int value = 0;
};

template <>
struct Fibonacci<1> {
    static const int value = 1;
};

constexpr int test()
{
    return Fibonacci<10>::value;
}

int testInt = test();