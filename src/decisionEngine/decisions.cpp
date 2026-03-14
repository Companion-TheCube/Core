/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "decisions.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <thread>

using namespace DecisionEngine;

namespace {

int64_t nowEpochMs()
{
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

nlohmann::json jsonValueFromString(const std::string& value)
{
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }
    if (lower == "true") return true;
    if (lower == "false") return false;

    char* end = nullptr;
    const auto integerValue = std::strtoll(value.c_str(), &end, 10);
    if (end != value.c_str() && *end == '\0') {
        return integerValue;
    }

    end = nullptr;
    const auto floatValue = std::strtod(value.c_str(), &end);
    if (end != value.c_str() && *end == '\0') {
        return floatValue;
    }

    return value;
}

std::string jsonToParameterString(const nlohmann::json& value)
{
    if (value.is_string()) return value.get<std::string>();
    if (value.is_boolean()) return value.get<bool>() ? "true" : "false";
    if (value.is_number_integer()) return std::to_string(value.get<long long>());
    if (value.is_number_float()) return std::to_string(value.get<double>());
    return value.dump();
}

std::string lookupParameter(const Parameters& parameters, const std::string& key)
{
    const auto it = parameters.find(key);
    if (it == parameters.end()) return {};
    return it->second;
}

} // namespace

DecisionEngineMain::DecisionEngineMain()
{
    audioQueue = std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>();
    remoteServerAPI = std::make_shared<TheCubeServer::TheCubeServerAPI>(audioQueue);

    intentRegistry = std::make_shared<IntentRegistry>();
    functionRegistry = std::make_shared<FunctionRegistry>();
    scheduler = std::make_shared<Scheduler>();
    triggerManager = std::make_shared<TriggerManager>(scheduler);
    personalityManager = std::make_shared<Personality::PersonalityManager>();

    if (intentRegistry) {
        intentRegistry->setFunctionRegistry(functionRegistry);
        try { intentRegistry->registerInterface(); } catch (...) { CubeLog::error("Failed to register IntentRegistry interface"); }
    }
    if (functionRegistry) {
        try { functionRegistry->registerInterface(); } catch (...) { CubeLog::error("Failed to register FunctionRegistry interface"); }
    }
    if (scheduler) {
        scheduler->setIntentRegistry(intentRegistry);
        scheduler->setFunctionRegistry(functionRegistry);
        try { scheduler->registerInterface(); } catch (...) { CubeLog::error("Failed to register Scheduler interface"); }
    }
    if (triggerManager) {
        triggerManager->setIntentRegistry(intentRegistry);
        triggerManager->setFunctionRegistry(functionRegistry);
        try { triggerManager->registerInterface(); } catch (...) { CubeLog::error("Failed to register TriggerManager interface"); }
    }
    if (personalityManager) {
        try { personalityManager->registerInterface(); } catch (...) { CubeLog::error("Failed to register PersonalityManager interface"); }
    }

    auto remoteIntentRecognition = std::make_shared<RemoteIntentRecognition>(intentRegistry);
    remoteIntentRecognition->setRemoteServerAPIObject(remoteServerAPI);
    intentRecognition = remoteIntentRecognition;
    if (scheduler) {
        scheduler->setIntentRecognition(intentRecognition);
    }

    auto remoteTranscriber = std::make_shared<RemoteTranscriber>();
    remoteTranscriber->setRemoteServerAPIObject(remoteServerAPI);
    transcriber = remoteTranscriber;

    lastDecisionResult.executionStatus = "idle";
    lastDecisionResult.timestampEpochMs = nowEpochMs();
}

DecisionEngineMain::~DecisionEngineMain()
{
    stop();
}

void DecisionEngineMain::start()
{
    std::scoped_lock lk(stateMutex);
    if (started) {
        return;
    }

    wakeAudioQueueHandle = SpeechIn::registerWakeAudioQueue(audioQueue);
    wakeWordCallbackHandle = SpeechIn::subscribeToWakeWordDetection([this]() {
        onWakeWordDetected();
    });
    transcriptionEventHandle = TranscriptionEvents::subscribe([this](const std::string& text, bool isFinal) {
        if (!isFinal) return;
        std::scoped_lock lock(stateMutex);
        latestTranscriptEvent = text;
    });

    if (scheduler) scheduler->start();
    if (triggerManager) triggerManager->start();
    started = true;
}

void DecisionEngineMain::stop()
{
    {
        std::scoped_lock lk(stateMutex);
        if (wakeWordCallbackHandle != std::numeric_limits<size_t>::max()) {
            SpeechIn::unsubscribeFromWakeWordDetection(wakeWordCallbackHandle);
            wakeWordCallbackHandle = std::numeric_limits<size_t>::max();
        }
        if (wakeAudioQueueHandle != std::numeric_limits<size_t>::max()) {
            SpeechIn::unregisterWakeAudioQueue(wakeAudioQueueHandle);
            wakeAudioQueueHandle = std::numeric_limits<size_t>::max();
        }
        if (transcriptionEventHandle != std::numeric_limits<size_t>::max()) {
            TranscriptionEvents::unsubscribe(transcriptionEventHandle);
            transcriptionEventHandle = std::numeric_limits<size_t>::max();
        }
        started = false;
    }

    if (auto rt = std::dynamic_pointer_cast<RemoteTranscriber>(transcriber)) {
        rt->interrupt();
    }
    stopTranscriptionConsumer();
    if (scheduler) scheduler->stop();
    if (triggerManager) triggerManager->stop();
}

void DecisionEngineMain::restart()
{
    stop();
    start();
}

void DecisionEngineMain::pause()
{
    if (scheduler) scheduler->pause();
}

void DecisionEngineMain::resume()
{
    if (scheduler) scheduler->resume();
}

void DecisionEngineMain::stopTranscriptionConsumer()
{
    if (transcriptionConsumerThread.joinable()) {
        transcriptionConsumerThread.request_stop();
        transcriptionConsumerThread.join();
    }
    transcription.reset();
}

void DecisionEngineMain::onWakeWordDetected()
{
    CubeLog::info("DecisionEngine: wake word detected");
    if (auto rt = std::dynamic_pointer_cast<RemoteTranscriber>(transcriber)) {
        rt->interrupt();
    }

    stopTranscriptionConsumer();
    transcription = transcriber ? transcriber->transcribeQueue(audioQueue) : nullptr;
    if (!transcription) {
        CubeLog::error("DecisionEngine: transcription queue was not created");
        return;
    }

    transcriptionConsumerThread = std::jthread([this](std::stop_token st) {
        using namespace std::chrono_literals;
        while (!st.stop_requested()) {
            if (!transcription || transcription->size() == 0) {
                std::this_thread::sleep_for(50ms);
                continue;
            }
            auto result = transcription->pop();
            if (!result || result->empty()) {
                std::this_thread::sleep_for(25ms);
                continue;
            }

            const auto turn = processTranscript(*result);
            recordTurnResult(turn);
            break;
        }
    });
}

nlohmann::json DecisionEngineMain::parametersToJson(const Parameters& parameters) const
{
    nlohmann::json args = nlohmann::json::object();
    for (const auto& [key, value] : parameters) {
        if (key == "capability_name") continue;
        args[key] = jsonValueFromString(value);
    }
    return args;
}

void DecisionEngineMain::applyCapabilityResultToIntent(const std::shared_ptr<Intent>& intent, const nlohmann::json& capabilityResult)
{
    if (!intent || !capabilityResult.is_object()) {
        return;
    }

    for (const auto& [key, value] : capabilityResult.items()) {
        intent->addParameter(key, jsonToParameterString(value));
    }
}

DecisionTurnResult DecisionEngineMain::executeIntent(const std::shared_ptr<Intent>& intent, const std::string& transcript)
{
    DecisionTurnResult result;
    result.transcript = transcript;
    result.timestampEpochMs = nowEpochMs();

    if (!intent) {
        result.executionStatus = "intent_not_found";
        result.error = "intent_not_found";
        return result;
    }

    result.intentName = intent->getIntentName();
    const auto parameters = intent->getParameters();
    const auto capabilityName = lookupParameter(parameters, "capability_name");

    if (!capabilityName.empty()) {
        const auto capabilityArgs = parametersToJson(parameters);
        result.capabilityResult = intent->runCapabilitySync(capabilityName, capabilityArgs, 4000);
        applyCapabilityResultToIntent(intent, result.capabilityResult);
        if (result.capabilityResult.is_object() && result.capabilityResult.contains("error")) {
            result.executionStatus = "capability_error";
            result.error = jsonToParameterString(result.capabilityResult["error"]);
            result.responseText = intent->getResponseString();
            return result;
        }
    }

    intent->execute();
    result.responseText = intent->getResponseString();
    result.executionStatus = "success";
    return result;
}

DecisionTurnResult DecisionEngineMain::processTranscript(const std::string& transcript)
{
    CubeLog::info("DecisionEngine: processing transcript: " + transcript);
    if (!intentRecognition) {
        return DecisionTurnResult {
            .transcript = transcript,
            .intentName = "",
            .executionStatus = "error",
            .responseText = "",
            .capabilityResult = nlohmann::json::object(),
            .error = "intent_recognition_unavailable",
            .timestampEpochMs = nowEpochMs()
        };
    }

    auto intent = intentRecognition->recognizeIntent("voice", transcript);
    return executeIntent(intent, transcript);
}

void DecisionEngineMain::recordTurnResult(const DecisionTurnResult& result)
{
    std::scoped_lock lk(stateMutex);
    lastDecisionResult = result;
}

nlohmann::json DecisionEngineMain::statusJson() const
{
    std::scoped_lock lk(stateMutex);
    return nlohmann::json({
        { "started", started },
        { "latestTranscriptEvent", latestTranscriptEvent },
        { "remoteServerStatus", static_cast<int>(remoteServerAPI ? remoteServerAPI->getServerStatus() : TheCubeServer::TheCubeServerAPI::ServerStatus::SERVER_STATUS_ERROR) },
        { "remoteServerError", static_cast<int>(remoteServerAPI ? remoteServerAPI->getServerError() : TheCubeServer::TheCubeServerAPI::ServerError::SERVER_ERROR_INTERNAL_ERROR) },
        { "lastDecisionResult", lastDecisionResult.toJson() }
    });
}

HttpEndPointData_t DecisionEngineMain::getHttpEndpointData()
{
    HttpEndPointData_t data;

    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            (void)req;
            res.status = 200;
            res.set_content(statusJson().dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "status",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Get decision engine runtime status"
    });

    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            (void)req;
            std::scoped_lock lk(stateMutex);
            res.status = 200;
            res.set_content(lastDecisionResult.toJson().dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "lastDecisionResult",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Get the last completed decision turn"
    });

    data.push_back({
        PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            std::string text;
            try {
                const auto body = nlohmann::json::parse(req.body);
                if (body.contains("text") && body["text"].is_string()) {
                    text = body["text"].get<std::string>();
                }
            } catch (...) {
                text = req.body;
            }

            if (text.empty()) {
                res.status = 400;
                res.set_content(nlohmann::json({ { "error", "text_required" } }).dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "text_required");
            }

            const auto result = processTranscript(text);
            recordTurnResult(result);
            res.status = 200;
            res.set_content(result.toJson().dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "runText",
        nlohmann::json({
            { "type", "object" },
            { "properties", { { "text", { { "type", "string" } } } } },
            { "required", nlohmann::json::array({ "text" }) }
        }),
        "Run a decision turn from plain text input"
    });

    return data;
}

std::future<std::string> DecisionEngine::modifyStringUsingAIForEmotionalState(
    const std::string& input,
    const std::vector<Personality::EmotionSimple>& emotions,
    const std::shared_ptr<TheCubeServer::TheCubeServerAPI>& remoteServerAPI,
    const std::function<void(std::string)>& progressCB)
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
                                          "Modify the following string to align with the given emotional state. Use the emotional state values to adjust tone, word choice, and phrasing appropriately while retaining the core message. Your response should be a JSON object with the key \"output\" and a value that is only the revised string. Other keys are permissible.\n\n"
                                          "Input:\n";
    std::string inputString = "String: \"" + input + "\"\nEmotional State: {";
    for (size_t i = 0; i < emotions.size(); i++) {
        inputString += Personality::emotionToString(emotions[i].emotion) + ": " + std::to_string(emotions[i].value);
        if (i < emotions.size() - 1) {
            inputString += ", ";
        }
    }
    inputString += "}";
    return remoteServerAPI->getChatResponseAsync(chatString + inputString, progressCB);
}
