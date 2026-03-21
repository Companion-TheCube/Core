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


// Intent, registry, and recognition strategies implementation. Remote recognition
// routes utterances to TheCubeServer for LLM-backed intent selection.
#include "intentRegistry.h"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace DecisionEngine {
namespace {
std::string trimWhitespace(const std::string& input)
{
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) ++start;
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) --end;
    return input.substr(start, end - start);
}

std::string stripQuotes(const std::string& input)
{
    if (input.size() >= 2) {
        char first = input.front();
        char last = input.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return input.substr(1, input.size() - 2);
        }
    }
    return input;
}

std::string toLowerAscii(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    for (unsigned char ch : input) out.push_back(static_cast<char>(std::tolower(ch)));
    return out;
}

std::string extractIntentName(const std::string& response)
{
    std::string trimmed = trimWhitespace(response);
    if (trimmed.empty()) return "";

    std::string maybeJson = trimmed;
    if (maybeJson.rfind("```", 0) == 0) {
        auto first = maybeJson.find('\n');
        auto last = maybeJson.rfind("```");
        if (first != std::string::npos && last != std::string::npos && last > first) {
            maybeJson = maybeJson.substr(first + 1, last - first - 1);
        }
    }
    maybeJson = trimWhitespace(maybeJson);

    try {
        auto j = nlohmann::json::parse(maybeJson);
        if (j.is_object() && j.contains("intentName") && j["intentName"].is_string()) {
            return j["intentName"].get<std::string>();
        }
        if (j.is_string()) {
            return j.get<std::string>();
        }
    } catch (...) {
    }

    return trimmed;
}

std::shared_ptr<Intent> matchIntentByName(const std::shared_ptr<IntentRegistry>& registry, const std::string& name)
{
    if (!registry) return nullptr;
    std::string cleaned = trimWhitespace(stripQuotes(name));
    if (cleaned.empty()) return nullptr;
    std::string lower = toLowerAscii(cleaned);
    if (lower == "null" || lower == "none" || lower == "no_intent" || lower == "no intent") return nullptr;

    auto direct = registry->getIntent(cleaned);
    if (direct) return direct;
    for (const auto& candidate : registry->getIntentNames()) {
        if (toLowerAscii(candidate) == lower) {
            return registry->getIntent(candidate);
        }
    }
    return nullptr;
}

std::shared_ptr<Intent> matchIntentFromResponseText(const std::shared_ptr<IntentRegistry>& registry, const std::string& response)
{
    if (!registry) return nullptr;
    std::string responseLower = toLowerAscii(response);
    for (const auto& candidate : registry->getIntentNames()) {
        if (candidate.empty()) continue;
        if (responseLower.find(toLowerAscii(candidate)) != std::string::npos) {
            return registry->getIntent(candidate);
        }
    }
    return nullptr;
}

std::string buildIntentPrompt(const std::string& utterance, const std::vector<std::shared_ptr<Intent>>& intents)
{
    nlohmann::json payload;
    payload["utterance"] = utterance;
    payload["intents"] = nlohmann::json::array();
    for (const auto& intent : intents) {
        if (!intent) continue;
        nlohmann::json item;
        item["name"] = intent->getIntentName();
        item["briefDesc"] = intent->getBriefDesc();
        payload["intents"].push_back(item);
    }

    std::string prompt =
        "You are an intent classifier for a robot assistant. "
        "Choose the single best intent name from the provided list. "
        "If no intent matches, return an empty string.\n"
        "Return ONLY a JSON object: {\"intentName\": \"...\"}.\n"
        "Data:\n";
    prompt += payload.dump(2);
    return prompt;
}

std::shared_ptr<Intent> runRemoteIntentRecognition(
    const std::shared_ptr<IntentRegistry>& registry,
    const std::shared_ptr<TheCubeServer::IRemoteConversationClient>& remoteConversationClient,
    const std::string& utterance)
{
    if (!registry || !remoteConversationClient) return nullptr;
    auto intents = registry->getRegisteredIntents();
    if (intents.empty()) return nullptr;

    std::string prompt = buildIntentPrompt(utterance, intents);
    const auto sessionId = remoteConversationClient->createConversationSession();
    if (!sessionId.has_value()) {
        CubeLog::error("RemoteIntentRecognition: failed to create conversation session");
        return nullptr;
    }
    auto fut = remoteConversationClient->getChatResponseAsync(*sessionId, prompt);
    std::string response = fut.get();
    if (response.empty()) return nullptr;

    std::string intentName = extractIntentName(response);
    auto intent = matchIntentByName(registry, intentName);
    if (!intent) intent = matchIntentFromResponseText(registry, response);
    return intent;
}

std::vector<std::string> configuredIntentRoots()
{
    std::vector<std::string> roots;
    const auto raw = Config::get("APP_INSTALL_ROOTS", "");
    if (raw.empty()) {
        roots.push_back("apps");
        return roots;
    }

    std::stringstream stream(raw);
    std::string part;
    while (std::getline(stream, part, ':')) {
        if (!part.empty()) {
            roots.push_back(part);
        }
    }
    return roots;
}

Parameters parametersFromJson(const nlohmann::json& value)
{
    Parameters parameters;
    if (!value.is_object()) {
        return parameters;
    }

    for (const auto& [key, jsonValue] : value.items()) {
        if (jsonValue.is_string()) {
            parameters[key] = jsonValue.get<std::string>();
        } else if (jsonValue.is_boolean()) {
            parameters[key] = jsonValue.get<bool>() ? "true" : "false";
        } else if (jsonValue.is_number_integer()) {
            parameters[key] = std::to_string(jsonValue.get<long long>());
        } else if (jsonValue.is_number_float()) {
            parameters[key] = std::to_string(jsonValue.get<double>());
        } else {
            parameters[key] = jsonValue.dump();
        }
    }

    return parameters;
}

Intent::IntentType intentTypeFromJson(const nlohmann::json& value)
{
    if (!value.is_string()) {
        return Intent::IntentType::COMMAND;
    }

    const auto lower = toLowerAscii(value.get<std::string>());
    return lower == "question" ? Intent::IntentType::QUESTION : Intent::IntentType::COMMAND;
}
} // namespace

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

// Helper: async function invocation via attached FunctionRegistry
void Intent::runFunctionAsync(const std::string& functionName, const nlohmann::json& args, std::function<void(const nlohmann::json&)> onComplete) const
{
    auto reg = functionRegistry.lock();
    if (!reg) {
        if (onComplete) onComplete(nlohmann::json({{"error","function_registry_unavailable"}}));
        return;
    }
    reg->runFunctionAsync(functionName, args, onComplete);
}

nlohmann::json Intent::runFunctionSync(const std::string& functionName, const nlohmann::json& args, uint32_t timeoutMs) const
{
    std::promise<nlohmann::json> prom;
    std::future<nlohmann::json> fut = prom.get_future();
    auto cb = [&prom](const nlohmann::json& res) mutable { try { prom.set_value(res); } catch(...) { } };
    runFunctionAsync(functionName, args, cb);
    if (fut.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::ready) {
        return fut.get();
    }
    return nlohmann::json({{"error","timeout"}});
}

void Intent::runCapabilityAsync(const std::string& capabilityName, const nlohmann::json& args, std::function<void(const nlohmann::json&)> onComplete) const
{
    auto reg = functionRegistry.lock();
    if (!reg) {
        if (onComplete) onComplete(nlohmann::json({{"error","function_registry_unavailable"}}));
        return;
    }
    reg->runCapabilityAsync(capabilityName, args, onComplete);
}

nlohmann::json Intent::runCapabilitySync(const std::string& capabilityName, const nlohmann::json& args, uint32_t timeoutMs) const
{
    std::promise<nlohmann::json> prom;
    std::future<nlohmann::json> fut = prom.get_future();
    auto cb = [&prom](const nlohmann::json& res) mutable { try { prom.set_value(res); } catch(...) { } };
    runCapabilityAsync(capabilityName, args, cb);
    if (fut.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::ready) {
        return fut.get();
    }
    return nlohmann::json({{"error","timeout"}});
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

    // Replace all referenced placeholders. Extra parameters are fine.
    for (const auto& param : parameters) {
        std::string placeholder = "${" + param.first + "}";
        size_t pos = temp.find(placeholder);
        while (pos != std::string::npos) {
            temp.replace(pos, placeholder.length(), param.second);
            pos = temp.find(placeholder, pos + param.second.length());
        }
    }
    if (temp.find("${") != std::string::npos) {
        CubeLog::warning("Unresolved placeholders remain in response string for intent: " + intentName);
    }
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

    auto makeIntent = [](std::string intentName,
                          std::string capabilityName,
                          std::string briefDesc,
                          std::string responseString,
                          Parameters parameters = Parameters(),
                          Intent::IntentType type = Intent::IntentType::COMMAND) {
        IntentCTorParams intent;
        intent.intentName = std::move(intentName);
        intent.parameters = std::move(parameters);
        intent.parameters["capability_name"] = std::move(capabilityName);
        intent.briefDesc = std::move(briefDesc);
        intent.responseString = std::move(responseString);
        intent.type = type;
        intent.action = [](const Parameters&, Intent) {};
        return intent;
    };

    intents.push_back(makeIntent(
        "core.ping",
        "core.ping",
        "User is checking whether TheCube is awake, online, listening, or responsive.",
        "Pong.",
        {},
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "core.get_time",
        "core.get_time",
        "User is asking for the current time, local time, or what time it is right now.",
        "The current time is ${time}.",
        { { "time", "" }, { "speak_result", "true" } },
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "core.get_date",
        "core.get_date",
        "User is asking for today's date, the current date, or what day it is.",
        "Today is ${date}.",
        { { "date", "" }, { "speak_result", "true" } },
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "core.get_datetime",
        "core.get_datetime",
        "User is asking for the current date and time together.",
        "It is ${datetime}.",
        { { "datetime", "" }, { "speak_result", "true" } },
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "core.ask_ai",
        "core.ask_ai",
        "User is asking a general informational, explanatory, or conversational question that does not map to a specific device action.",
        "${answer}",
        { { "answer", "" } },
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "core.create_reminder",
        "core.create_reminder",
        "User wants TheCube to create a reminder for a future time, date, or delay.",
        "${summary}",
        { { "summary", "" }, { "notificationId", "" }, { "scheduledForEpochMs", "" } },
        Intent::IntentType::COMMAND));

    intents.push_back(makeIntent(
        "core.create_alarm",
        "core.create_alarm",
        "User wants TheCube to set an alarm for a future time or delay.",
        "${summary}",
        { { "summary", "" }, { "notificationId", "" }, { "scheduledForEpochMs", "" } },
        Intent::IntentType::COMMAND));

    intents.push_back(makeIntent(
        "core.list_reminders",
        "core.list_reminders",
        "User is asking what reminders are scheduled or what reminders are coming up.",
        "${summary}",
        { { "summary", "" }, { "count", "" } },
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "core.list_alarms",
        "core.list_alarms",
        "User is asking what alarms are active or currently scheduled.",
        "${summary}",
        { { "summary", "" }, { "count", "" } },
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "core.cancel_notification",
        "core.cancel_notification",
        "User wants to cancel, dismiss, or clear a reminder or alarm.",
        "${summary}",
        { { "summary", "" } },
        Intent::IntentType::COMMAND));

    intents.push_back(makeIntent(
        "core.snooze_alarm",
        "core.snooze_alarm",
        "User wants to snooze the currently ringing or active alarm.",
        "${summary}",
        { { "summary", "" } },
        Intent::IntentType::COMMAND));

    intents.push_back(makeIntent(
        "apps.list_installed",
        "apps.list_installed",
        "User is asking what apps are installed, available, or on this device.",
        "Installed apps: ${appNamesString}.",
        { { "appNamesString", "" }, { "count", "" } },
        Intent::IntentType::QUESTION));

    intents.push_back(makeIntent(
        "audio.toggle_sound",
        "audio.toggle_sound",
        "User wants the sound toggled, muted, unmuted, or switched to the opposite state.",
        "Audio output toggled.",
        {},
        Intent::IntentType::COMMAND));

    intents.push_back(makeIntent(
        "audio.set_sound_on",
        "audio.set_sound",
        "User wants sound turned on, enabled, or unmuted explicitly.",
        "Audio output is on.",
        { { "soundOn", "true" } },
        Intent::IntentType::COMMAND));

    intents.push_back(makeIntent(
        "audio.set_sound_off",
        "audio.set_sound",
        "User wants sound turned off, disabled, or muted explicitly.",
        "Audio output is off.",
        { { "soundOn", "false" } },
        Intent::IntentType::COMMAND));

    return intents;
}

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
    auto registry = intentRegistry;
    auto conversationClient = remoteConversationClient;
    if (!registry || !conversationClient)
        return false;
    std::thread([registry, conversationClient, intentString, callback = std::move(callback)]() mutable {
        auto intent = runRemoteIntentRecognition(registry, conversationClient, intentString);
        if (callback) {
            try {
                callback(intent);
            } catch (...) {
                CubeLog::error("RemoteIntentRecognition: callback exception");
            }
        }
    }).detach();
    return true;
}

std::shared_ptr<Intent> RemoteIntentRecognition::recognizeIntent(const std::string& name, const std::string& intentString)
{
    (void)name;
    return runRemoteIntentRecognition(intentRegistry, remoteConversationClient, intentString);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IntentRegistry - class that contains the intents that are registered with the system
// and exposes read-only inspection endpoints for V1.
IntentRegistry::IntentRegistry()
{
    for (auto& intent : getSystemIntents()) {
        auto newIntent = std::make_shared<Intent>(intent);
        if (!registerIntent(intent.intentName, newIntent))
            CubeLog::error("Failed to register intent: " + intent.intentName);
    }
}

void IntentRegistry::loadIntentManifests()
{
    if (manifestIntentsLoaded || !functionRegistry) {
        return;
    }

    auto loadManifest = [this](const std::filesystem::path& filePath) {
        try {
            std::ifstream ifs(filePath);
            if (!ifs.good()) {
                CubeLog::error("IntentRegistry: failed to open intent manifest " + filePath.string());
                return;
            }

            const auto manifest = nlohmann::json::parse(ifs);
            const auto intentName = manifest.value("intentName", "");
            if (intentName.empty()) {
                CubeLog::error("IntentRegistry: manifest missing intentName: " + filePath.string());
                return;
            }

            std::string capabilityName = manifest.value("capabilityName", "");
            if (capabilityName.empty()) {
                capabilityName = manifest.value("capability_name", "");
            }
            if (capabilityName.empty()) {
                CubeLog::error("IntentRegistry: manifest missing capabilityName: " + filePath.string());
                return;
            }

            if (!functionRegistry->findCapability(capabilityName)) {
                CubeLog::error(
                    "IntentRegistry: manifest references unknown capability '"
                    + capabilityName
                    + "': "
                    + filePath.string());
                return;
            }

            Parameters parameters;
            if (manifest.contains("parameters")) {
                parameters = parametersFromJson(manifest["parameters"]);
            } else if (manifest.contains("defaultParameters")) {
                parameters = parametersFromJson(manifest["defaultParameters"]);
            }
            parameters["capability_name"] = capabilityName;

            if (manifest.value("speakResult", false)) {
                parameters["speak_result"] = "true";
            }

            IntentCTorParams params;
            params.intentName = intentName;
            params.parameters = std::move(parameters);
            params.briefDesc = manifest.value("briefDesc", "");
            params.responseString = manifest.value("responseString", "");
            params.type = intentTypeFromJson(manifest.value("type", "command"));
            params.action = [](const Parameters&, Intent) {};

            auto intent = std::make_shared<Intent>(params);
            if (!registerIntent(intentName, intent)) {
                CubeLog::error("IntentRegistry: duplicate intent manifest name '" + intentName + "'");
            }
        } catch (const std::exception& e) {
            CubeLog::error(std::string("IntentRegistry: failed to parse intent manifest: ") + e.what());
        }
    };

    const std::vector<std::filesystem::path> searchRoots = { std::filesystem::path("data/intents") };
    for (const auto& root : searchRoots) {
        std::error_code ec;
        if (!std::filesystem::exists(root, ec)) {
            continue;
        }
        for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
            if (ec || !entry.is_regular_file()) {
                continue;
            }
            loadManifest(entry.path());
        }
    }

    for (const auto& root : configuredIntentRoots()) {
        std::error_code ec;
        if (!std::filesystem::exists(root, ec)) {
            continue;
        }
        for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
            if (ec || !entry.is_directory()) {
                continue;
            }
            const auto intentsDir = entry.path() / "intents";
            if (!std::filesystem::exists(intentsDir, ec)) {
                continue;
            }
            for (const auto& file : std::filesystem::directory_iterator(intentsDir, ec)) {
                if (ec || !file.is_regular_file()) {
                    continue;
                }
                loadManifest(file.path());
            }
        }
    }

    manifestIntentsLoaded = true;
}

void IntentRegistry::setFunctionRegistry(std::shared_ptr<FunctionRegistry> registry)
{
    functionRegistry = std::move(registry);
    for (auto & kv : intentMap) {
        auto &intent = kv.second;
        if (intent) intent->setFunctionRegistry(functionRegistry);
    }
    loadIntentManifests();
}

bool IntentRegistry::registerIntent(const std::string& intentName, const std::shared_ptr<Intent> intent)
{
    // Check if the intent is already registered
    if (intentMap.find(intentName) != intentMap.end())
        return false;
    if (intent && functionRegistry) {
        intent->setFunctionRegistry(functionRegistry);
    }
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

    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            (void)req;
            nlohmann::json intents = nlohmann::json::array();
            for (const auto& intent : getRegisteredIntents()) {
                if (!intent) continue;
                intents.push_back({
                    { "intentName", intent->getIntentName() },
                    { "briefDesc", intent->getBriefDesc() },
                    { "responseString", intent->getResponseString() }
                });
            }
            res.status = 200;
            res.set_content(intents.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "listIntents",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "List registered intents"
    });

    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            const auto intentName = req.has_param("intentName") ? req.get_param_value("intentName") : "";
            auto intent = getIntent(intentName);
            if (!intent) {
                res.status = 404;
                res.set_content(nlohmann::json({ { "error", "intent_not_found" } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "intent_not_found");
            }

            nlohmann::json out = {
                { "intentName", intent->getIntentName() },
                { "briefDesc", intent->getBriefDesc() },
                { "responseString", intent->getResponseString() },
                { "parameters", intent->getParameters() }
            };
            res.status = 200;
            res.set_content(out.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "getIntent",
        nlohmann::json({
            { "type", "object" },
            { "properties", { { "intentName", { { "type", "string" } } } } },
            { "required", nlohmann::json::array({ "intentName" }) }
        }),
        "Get a registered intent"
    });

    return data;
}

constexpr std::string IntentRegistry::getInterfaceName() const
{
    return "IntentRegistry";
}



}
