/*
██╗███╗   ██╗████████╗███████╗███╗   ██╗████████╗██████╗ ███████╗ ██████╗ ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗██████╗ ██████╗ 
██║████╗  ██║╚══██╔══╝██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔══██╗╚██╗ ██╔╝██╔════╝██╔══██╗██╔══██╗
██║██╔██╗ ██║   ██║   █████╗  ██╔██╗ ██║   ██║   ██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   ██████╔╝ ╚████╔╝ ██║     ██████╔╝██████╔╝
██║██║╚██╗██║   ██║   ██╔══╝  ██║╚██╗██║   ██║   ██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══██╗  ╚██╔╝  ██║     ██╔═══╝ ██╔═══╝ 
██║██║ ╚████║   ██║   ███████╗██║ ╚████║   ██║   ██║  ██║███████╗╚██████╔╝██║███████║   ██║   ██║  ██║   ██║██╗╚██████╗██║     ██║     
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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


// Intent, registry, and recognition strategies implementation. Local recognition
// uses a simple token/parameter-name overlap heuristic; remote is a stub for
// TheCubeServer-backed recognition.
#include "intentRegistry.h"

namespace DecisionEngine {
// Intent - class that contains the intent data including the action to take

/**
 * @brief Construct a new Intent object with no parameters
 * briefDesc and responseString will be set to empty strings. Call setBriefDesc and setResponseString to set these values. *
 * @param intentName
 * @param action
 */
Intent::Intent(const std::string& intentName, const Action& action)
{
    this->intentName = intentName;
    this->action = action;
    this->briefDesc = "";
    this->responseString = "";
    this->type = Intent::IntentType::COMMAND;
}

/**
 * @brief Construct a new Intent object
 * briefDesc and responseString will be set to empty strings. Call setBriefDesc and setResponseString to set these values. *
 * @param intentName
 * @param action
 * @param parameters
 */
Intent::Intent(const std::string& intentName, const Action& action, const Parameters& parameters)
{
    this->intentName = intentName;
    this->action = action;
    this->parameters = parameters;
    this->briefDesc = "";
    this->responseString = "";
    this->type = Intent::IntentType::COMMAND;
}

/**
 * @brief Construct a new Intent object
 *
 * @param intentName The name of the intent
 * @param action The action to take when the intent is executed
 * @param parameters The parameters for the intent. For example, "What time is it?" would have a parameter of "time" with the value of the current time.
 * @param briefDesc A brief description of the intent. For example, "Get the current time"
 * @param responseString This is the string that will be returned to the user when the intent is executed.
 * This string can contain placeholders for the parameters. The placeholders will be replaced with the actual values.
 * The placeholders should be in the format ${parameterName}
 */
Intent::Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString)
{
    this->intentName = intentName;
    this->action = action;
    this->parameters = parameters;
    this->briefDesc = briefDesc;
    this->responseString = responseString;
    this->type = Intent::IntentType::COMMAND;
}

/**
 * @brief Construct a new Intent object
 *
 * @param intentName The name of the intent
 * @param action The action to take when the intent is executed
 * @param parameters The parameters for the intent. For example, "What time is it?" would have a parameter of "time" with the value of the current time.
 * @param briefDesc A brief description of the intent. For example, "Get the current time"
 * @param responseString The string that will be returned to the user when the intent is executed. This string can contain placeholders for the parameters. For example, "The current time is ${time}"
 * @param type The type of the intent. This can be either a question or a command.
 */
Intent::Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString, Intent::IntentType type)
{
    this->intentName = intentName;
    this->action = action;
    this->parameters = parameters;
    this->briefDesc = briefDesc;
    this->responseString = responseString;
    this->type = type;
}

Intent::Intent(IntentCTorParams params)
{
    this->intentName = params.intentName;
    this->action = params.action;
    this->parameters = params.parameters;
    this->briefDesc = params.briefDesc;
    this->responseString = params.responseString;
    this->type = params.type;
}

const std::string& Intent::getIntentName() const
{
    return intentName;
}

const Parameters& Intent::getParameters() const
{
    return parameters;
}

void Intent::setParameters(const Parameters& parameters)
{
    this->parameters = parameters;
}

bool Intent::setParameter(const std::string& key, const std::string& value)
{
    if (parameters.find(key) != parameters.end()) {
        parameters[key] = value;
        return true;
    }
    return false;
}

void Intent::addParameter(const std::string& key, const std::string& value)
{
    parameters[key] = value;
}

void Intent::execute() const
{
    if (action) {
        // TODO: add TTS support
        action(parameters, *this);
    } 
    // DecisionEngineError has been removed. opt for a simple log message instead
    //throw DecisionEngine::DecisionEngineError("No action set for intent: " + intentName, DecisionEngine::DecisionErrorType::NO_ACTION_SET);
    else {
        CubeLog::error("No action set for intent: " + intentName);
    }
}

const std::string Intent::serialize()
{
    nlohmann::json j;
    j["intentName"] = intentName;
    j["parameters"] = parameters;
    return j.dump();
}

/**
 * @brief Convert a JSON object to an Intent::Action. USed to convert a serialized intent to a callable action. *
 * @param action_json
 * @return Intent::Action
 */
// Convert a JSON description into an executable Action. Placeholder for now;
// a production implementation would bind to registered functions or scripts.
Action convertJsonToAction(const nlohmann::json& action_json)
{
    // TODO: Implement this
    return nullptr;
}

std::shared_ptr<Intent> Intent::deserialize(const std::string& serializedIntent)
{
    // TODO: This needs checks to ensure properly formatted JSON
    nlohmann::json j = nlohmann::json::parse(serializedIntent);
    std::string intentName = j["intentName"];
    Parameters parameters = j["parameters"];
    Action action = convertJsonToAction(j["action"]);
    return std::make_shared<Intent>(intentName, action, parameters);
}

const std::string& Intent::getBriefDesc() const
{
    return briefDesc;
}

const std::string Intent::getResponseString() const
{
    std::string temp = responseString;
    if (responseStringScored.size() > 0 && personalityManager) {
        auto score = personalityManager->calculateEmotionalMatchScore(emotionRanges);
        auto scoreIndex = Personality::interpretScore(score);
        if (scoreIndex < responseStringScored.size())
            temp = responseStringScored[scoreIndex];
        else {
            CubeLog::warning("Score index out of range for intent: " + intentName);
        }
    }

    // Parse the response string and replace the placeholders with the actual values
    bool allParamsFound = true;
    for (auto& param : parameters) {
        std::string placeholder = "${" + param.first + "}";
        size_t pos = temp.find(placeholder);
        if (pos != std::string::npos)
            temp.replace(pos, placeholder.length(), param.second);
        else
            allParamsFound = false;
    }
    if (!allParamsFound)
        CubeLog::warning("Not all parameters found in response string for intent: " + intentName);
    return temp;
}

void Intent::setResponseString(const std::string& responseString)
{
    this->responseString = responseString;
}

void Intent::setScoredResponseStrings(const std::vector<std::string>& responseStrings)
{
    if (responseStrings.size() > 5) {
        CubeLog::warning("Too many scored response strings for intent: " + intentName);
        CubeLog::warning("Only the first 5 will be used.");
    }
    for (size_t i = 0; i < responseStrings.size() && i < 5; i++) {
        responseStringScored.push_back(responseStrings[i]);
    }
}

void Intent::setScoredResponseString(const std::string& responseString, int score)
{
    if (score < 0 || score > 5) {
        CubeLog::warning("Score out of range for intent: " + intentName);
        CubeLog::warning("Setting score to " + std::string(score < 0 ? "0" : "5"));
        score = score < 0 ? 0 : 5;
    }
    std::string t = "";
    if (responseStringScored.size() > 0)
        t = responseStringScored.at(responseStringScored.size() - 1);
    while (responseStringScored.size() <= score) {
        responseStringScored.push_back(t);
    }
    responseStringScored.at(score) = responseString;
}

void Intent::setEmotionalScoreRanges(const std::vector<Personality::EmotionRange>& emotionRanges)
{
    this->emotionRanges = emotionRanges;
}

void Intent::setEmotionScoreRange(const Personality::EmotionRange& emotionRange)
{
    auto emote = emotionRange.emotion;
    bool found = false;
    for (auto& range : emotionRanges) {
        if (range.emotion == emote) {
            range = emotionRange;
            found = true;
            break;
        }
    }
    if (!found) {
        CubeLog::warning("Emotion range not found for intent: " + intentName);
        CubeLog::warning("Adding new range for emotion: " + Personality::emotionToString(emote));
        emotionRanges.push_back(emotionRange);
    }
}

void Intent::setSerializedData(const std::string& serializedData)
{
    this->serializedData = serializedData;
}

const std::string& Intent::getSerializedData() const
{
    return serializedData;
}

void Intent::setBriefDesc(const std::string& briefDesc)
{
    this->briefDesc = briefDesc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get all the built-in system intents
 * @return std::vector<IntentCTorParams>
 */
// Define built-in system intents the engine ships with. Apps can register
// additional intents at runtime via the IntentRegistry API.
std::vector<IntentCTorParams> getSystemIntents()
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

// Very lightweight recognizer: tokenizes the input string and counts matches
// against the parameter names of an existing intent. Returns null if no tokens
// match any parameter name. This is a stopgap until a better matcher exists.
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
// On construction, register system intents. Also performs a temporary HTTP
// request to dummyjson.com for connectivity testing (TODO: remove).
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



}
