/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "decisions.h"
#include "../audio/audioOutput.h"
#include "../gui/gui.h"
#include "nlohmann/json-schema.hpp"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace DecisionEngine;

namespace {

std::string makeOpenAiToolName(const std::string& rawName)
{
    std::string normalized;
    normalized.reserve(rawName.size() * 2);
    for (unsigned char ch : rawName) {
        if (std::isalnum(ch) || ch == '_' || ch == '-') {
            normalized.push_back(static_cast<char>(ch));
            continue;
        }

        std::ostringstream escaped;
        escaped << "_x" << std::hex << std::nouppercase << static_cast<int>(ch) << "_";
        normalized += escaped.str();
    }

    if (normalized.empty()) {
        return "tool";
    }
    if (!(std::isalpha(static_cast<unsigned char>(normalized.front())) || normalized.front() == '_')) {
        normalized = "tool_" + normalized;
    }
    return normalized;
}

std::string summarizeJsonForLog(const nlohmann::json& value, size_t maxLen = 400)
{
    std::string dump;
    try {
        dump = value.dump();
    } catch (...) {
        dump = "<json_dump_failed>";
    }
    if (dump.size() <= maxLen) {
        return dump;
    }
    return dump.substr(0, maxLen) + "...";
}

int64_t nowEpochMs()
{
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

std::string nowIsoLocal()
{
    const auto now = std::chrono::system_clock::now();
    const auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTm {};
#if defined(_WIN32)
    localtime_s(&localTm, &nowTime);
#else
    localtime_r(&nowTime, &localTm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
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

bool parseBoolString(const std::string& value)
{
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }
    return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

std::string toLowerTrimmedAscii(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    std::string out;
    out.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(value[i]))));
    }
    return out;
}

bool startsWith(const std::string& value, const std::string& prefix)
{
    return value.rfind(prefix, 0) == 0;
}

bool shouldFallbackToAskAi(const std::string& transcript)
{
    const auto normalized = toLowerTrimmedAscii(transcript);
    if (normalized.empty()) {
        return false;
    }
    if (!normalized.empty() && normalized.back() == '?') {
        return true;
    }

    std::istringstream iss(normalized);
    std::string firstToken;
    iss >> firstToken;
    static const std::vector<std::string> questionStarters = {
        "who", "what", "when", "where", "why", "how",
        "is", "are", "can", "could", "would", "should",
        "do", "does", "did"
    };
    if (std::find(questionStarters.begin(), questionStarters.end(), firstToken) != questionStarters.end()) {
        return true;
    }

    return startsWith(normalized, "tell me")
        || startsWith(normalized, "explain")
        || startsWith(normalized, "describe");
}

std::string localTimezoneId()
{
    if (const char* tz = std::getenv("TZ")) {
        std::string value = tz;
        if (!value.empty()) {
            return value;
        }
    }

    {
        std::ifstream timezoneFile("/etc/timezone");
        std::string timezone;
        if (timezoneFile.good() && std::getline(timezoneFile, timezone)) {
            timezone = toLowerTrimmedAscii(timezone) == "utc" ? "UTC" : timezone;
            if (!timezone.empty()) {
                return timezone;
            }
        }
    }

    std::error_code ec;
    if (std::filesystem::is_symlink("/etc/localtime", ec)) {
        const auto link = std::filesystem::read_symlink("/etc/localtime", ec).string();
        const auto marker = link.find("zoneinfo/");
        if (marker != std::string::npos) {
            return link.substr(marker + 9);
        }
    }

    return "UTC";
}

bool validateJsonAgainstSchema(const nlohmann::json& value, const nlohmann::json& schema, std::string& error)
{
    try {
        if (!schema.is_object() || schema.empty()) {
            return true;
        }
        nlohmann::json rootSchema = schema;
        nlohmann::json_schema::json_validator validator(
            [rootSchema](const nlohmann::json_uri& uri, nlohmann::json& resolved) mutable {
                const auto ref = uri.to_string();
                if (!ref.empty() && ref[0] == '#') {
                    resolved = rootSchema.at(nlohmann::json::json_pointer(ref.substr(1)));
                    return;
                }
                throw std::runtime_error("External schema references are not supported for voiceInputSchema");
            });
        validator.set_root_schema(rootSchema);
        validator.validate(value);
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

nlohmann::json effectiveVoiceSchema(const std::shared_ptr<Intent>& intent, const DecisionEngine::CapabilitySpec* capability)
{
    (void)intent;
    if (!capability || !capability->voiceEnabled) {
        return nlohmann::json::object();
    }

    nlohmann::json schema = capability->voiceInputSchema.is_object()
        ? capability->voiceInputSchema
        : nlohmann::json::object();
    if (!schema.contains("type")) {
        schema["type"] = "object";
    }
    if (!schema.contains("properties") || !schema["properties"].is_object()) {
        schema["properties"] = nlohmann::json::object();
    }
    if (!schema.contains("additionalProperties")) {
        schema["additionalProperties"] = false;
    }
    return schema;
}

nlohmann::json buildVoiceToolCatalogue(
    const std::shared_ptr<IntentRegistry>& intentRegistry,
    const std::shared_ptr<FunctionRegistry>& functionRegistry)
{
    nlohmann::json tools = nlohmann::json::array();
    if (!intentRegistry || !functionRegistry) {
        return tools;
    }

    for (const auto& intent : intentRegistry->getRegisteredIntents()) {
        if (!intent) {
            continue;
        }
        const auto capabilityName = lookupParameter(intent->getParameters(), "capability_name");
        if (capabilityName.empty()) {
            continue;
        }
        const auto* capability = functionRegistry->findCapability(capabilityName);
        if (!capability || !capability->voiceEnabled || !capability->enabled) {
            continue;
        }
        tools.push_back({
            { "name", makeOpenAiToolName(intent->getIntentName()) },
            { "intentName", intent->getIntentName() },
            { "description", intent->getBriefDesc() },
            { "parameters", effectiveVoiceSchema(intent, capability) }
        });
    }
    return tools;
}

bool shouldSpeakForGeneralAiAnswer()
{
    try {
        const auto mode = GlobalSettings::getSettingOfType<std::string>(GlobalSettings::SettingType::GENERAL_AI_RESPONSE_MODE);
        return mode == "popupAndSpeech" || mode == "speechFirst";
    } catch (...) {
        return false;
    }
}

std::string turnStateName(DecisionEngineMain::TurnState state)
{
    switch (state) {
    case DecisionEngineMain::TurnState::IDLE:
        return "idle";
    case DecisionEngineMain::TurnState::WAKE_ACKNOWLEDGED:
        return "wake_acknowledged";
    case DecisionEngineMain::TurnState::LISTENING:
        return "listening";
    case DecisionEngineMain::TurnState::TRANSCRIPT_STREAMING:
        return "transcript_streaming";
    case DecisionEngineMain::TurnState::FINAL_TRANSCRIPT_PENDING_INTENT:
        return "final_transcript_pending_intent";
    case DecisionEngineMain::TurnState::INTENT_EXECUTING:
        return "intent_executing";
    case DecisionEngineMain::TurnState::RESULT_PRESENTED:
        return "result_presented";
    }
    return "unknown";
}

std::filesystem::path wakeSoundPath()
{
    return std::filesystem::path("data") / "wake_sounds" / "song (3).wav";
}

class TurnPresentationController {
public:
    void showListening()
    {
        generation_.fetch_add(1, std::memory_order_relaxed);
        GUI::showMessageBox("TheCube", "Listening...");
    }

    void updateTranscript(const std::string& text)
    {
        if (text.empty()) {
            return;
        }
        GUI::showMessageBox("TheCube", text);
    }

    void showResult(const std::string& text)
    {
        if (text.empty()) {
            return;
        }
        generation_.fetch_add(1, std::memory_order_relaxed);
        GUI::showMessageBox("TheCube", text);
    }

    void hide()
    {
        generation_.fetch_add(1, std::memory_order_relaxed);
        GUI::hideMessageBox();
    }

    void scheduleHide(std::chrono::milliseconds delay)
    {
        const auto generation = generation_.load(std::memory_order_relaxed);
        std::thread([generation, delay, this]() {
            std::this_thread::sleep_for(delay);
            if (generation_.load(std::memory_order_relaxed) != generation) {
                return;
            }
            GUI::hideMessageBox();
        }).detach();
    }

private:
    std::atomic<uint64_t> generation_ { 0 };
};

TurnPresentationController& presentationController()
{
    static TurnPresentationController controller;
    return controller;
}

} // namespace

DecisionEngineMain::DecisionEngineMain()
{
    audioQueue = std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>();
    remoteServerAPI = std::make_shared<TheCubeServer::TheCubeServerAPI>(audioQueue);

    intentRegistry = std::make_shared<IntentRegistry>();
    functionRegistry = std::make_shared<FunctionRegistry>();
    scheduler = std::make_shared<Scheduler>();
    notificationCenter = std::make_shared<NotificationCenter>();
    triggerManager = std::make_shared<TriggerManager>(scheduler);
    personalityManager = std::make_shared<Personality::PersonalityManager>();

    if (intentRegistry) {
        intentRegistry->setFunctionRegistry(functionRegistry);
        try { intentRegistry->registerInterface(); } catch (...) { CubeLog::error("Failed to register IntentRegistry interface"); }
    }
    if (functionRegistry) {
        functionRegistry->setRemoteConversationClient(remoteServerAPI);
        functionRegistry->setNotificationCenter(notificationCenter);
        try { functionRegistry->registerInterface(); } catch (...) { CubeLog::error("Failed to register FunctionRegistry interface"); }
    }
    if (scheduler) {
        scheduler->setIntentRegistry(intentRegistry);
        scheduler->setFunctionRegistry(functionRegistry);
        try { scheduler->registerInterface(); } catch (...) { CubeLog::error("Failed to register Scheduler interface"); }
    }
    if (notificationCenter) {
        notificationCenter->setScheduler(scheduler);
        NotificationCenter::setSharedInstance(notificationCenter);
        try { notificationCenter->registerInterface(); } catch (...) { CubeLog::error("Failed to register NotificationCenter interface"); }
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
        handleTranscriptEvent(text, isFinal);
    });

    if (scheduler) scheduler->start();
    if (notificationCenter) notificationCenter->start();
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
        activeTranscriptPreview.clear();
        latestTranscriptEvent.clear();
        turnState = TurnState::IDLE;
    }

    if (auto rt = std::dynamic_pointer_cast<RemoteTranscriber>(transcriber)) {
        rt->interrupt();
    }
    stopTranscriptionConsumer();
    hideTurnUi();
    if (notificationCenter) notificationCenter->stop();
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

void DecisionEngineMain::setTurnState(TurnState newState)
{
    std::scoped_lock lock(stateMutex);
    turnState = newState;
}

void DecisionEngineMain::hideTurnUi()
{
    presentationController().hide();
}

void DecisionEngineMain::showListeningUi()
{
    presentationController().showListening();
}

void DecisionEngineMain::handleTranscriptEvent(const std::string& text, bool isFinal)
{
    if (text.empty()) {
        return;
    }

    bool acceptEvent = false;
    {
        std::scoped_lock lock(stateMutex);
        acceptEvent = started
            && turnState != TurnState::IDLE
            && turnState != TurnState::INTENT_EXECUTING
            && turnState != TurnState::RESULT_PRESENTED;
        if (!acceptEvent) {
            return;
        }
        latestTranscriptEvent = text;
        activeTranscriptPreview = text;
        if (isFinal) {
            turnState = TurnState::FINAL_TRANSCRIPT_PENDING_INTENT;
        } else {
            turnState = TurnState::TRANSCRIPT_STREAMING;
        }
    }

    presentationController().updateTranscript(text);
}

void DecisionEngineMain::presentTurnResult(const DecisionTurnResult& result)
{
    const std::string message = !result.responseText.empty()
        ? result.responseText
        : (!result.error.empty() ? result.error : "Done.");
    CubeLog::info(
        "DecisionEngine: presenting result"
        " status=" + result.executionStatus
        + ", intent=" + result.intentName
        + ", error=" + result.error
        + ", message=" + message);
    presentationController().showResult(message);

    if (result.speakResult && !message.empty() && functionRegistry) {
        functionRegistry->runCapabilityAsync(
            "core.speak_text",
            nlohmann::json({ { "text", message } }),
            nullptr);
    }

    setTurnState(TurnState::RESULT_PRESENTED);
    presentationController().scheduleHide(std::chrono::milliseconds(4000));
}

void DecisionEngineMain::onWakeWordDetected()
{
    CubeLog::info("DecisionEngine: wake word detected");
    if (auto rt = std::dynamic_pointer_cast<RemoteTranscriber>(transcriber)) {
        rt->interrupt();
    }

    stopTranscriptionConsumer();
    setTurnState(TurnState::WAKE_ACKNOWLEDGED);
    {
        std::scoped_lock lock(stateMutex);
        activeTranscriptPreview.clear();
        latestTranscriptEvent.clear();
    }
    AudioOutput::playFileAsync(wakeSoundPath());
    showListeningUi();
    setTurnState(TurnState::LISTENING);
    transcription = transcriber ? transcriber->transcribeQueue(audioQueue) : nullptr;
    if (!transcription) {
        CubeLog::error("DecisionEngine: transcription queue was not created");
        hideTurnUi();
        setTurnState(TurnState::IDLE);
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
            if (!result) {
                std::this_thread::sleep_for(25ms);
                continue;
            }
            if (result->empty()) {
                CubeLog::warning("DecisionEngine: transcription session ended without a final transcript");
                recordTurnResult(DecisionTurnResult {
                    .transcript = "",
                    .intentName = "",
                    .executionStatus = "transcription_failed",
                    .responseText = "",
                    .capabilityResult = nlohmann::json::object(),
                    .error = "transcription_failed",
                    .speakResult = false,
                    .timestampEpochMs = nowEpochMs()
                });
                hideTurnUi();
                setTurnState(TurnState::IDLE);
                break;
            }

            setTurnState(TurnState::FINAL_TRANSCRIPT_PENDING_INTENT);
            const auto turn = processTranscript(*result);
            recordTurnResult(turn);
            presentTurnResult(turn);
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

DecisionTurnResult DecisionEngineMain::executeIntent(const std::shared_ptr<Intent>& intent, const std::string& transcript, const nlohmann::json& resolvedArgs)
{
    DecisionTurnResult result;
    result.transcript = transcript;
    result.timestampEpochMs = nowEpochMs();
    setTurnState(TurnState::INTENT_EXECUTING);

    if (!intent) {
        CubeLog::warning("DecisionEngine: executeIntent called with null intent for transcript: " + transcript);
        result.executionStatus = "intent_not_found";
        result.error = "intent_not_found";
        result.responseText = "I couldn't match that request to an available action.";
        return result;
    }

    result.intentName = intent->getIntentName();
    const auto parameters = intent->getParameters();
    const auto capabilityName = lookupParameter(parameters, "capability_name");
    result.speakResult = parseBoolString(lookupParameter(parameters, "speak_result"));

    if (!capabilityName.empty()) {
        if (const auto* capability = functionRegistry ? functionRegistry->findCapability(capabilityName) : nullptr) {
            const auto schema = effectiveVoiceSchema(intent, capability);
            std::string validationError;
            const auto argsToValidate = resolvedArgs.is_object() ? resolvedArgs : nlohmann::json::object();
            if (!validateJsonAgainstSchema(argsToValidate, schema, validationError)) {
                CubeLog::warning(
                    "DecisionEngine: capability argument validation failed"
                    " intent=" + result.intentName
                    + ", capability=" + capabilityName
                    + ", args=" + summarizeJsonForLog(argsToValidate)
                    + ", schema=" + summarizeJsonForLog(schema)
                    + ", error=" + validationError);
                result.executionStatus = "capability_validation_error";
                result.error = validationError;
                result.responseText = "I understood the request, but I couldn't determine the required details.";
                return result;
            }
        }

        auto capabilityArgs = parametersToJson(parameters);
        if (resolvedArgs.is_object()) {
            for (const auto& [key, value] : resolvedArgs.items()) {
                capabilityArgs[key] = value;
            }
        }
        if (!capabilityArgs.contains("transcript")) {
            capabilityArgs["transcript"] = transcript;
        }

        CubeLog::info(
            "DecisionEngine: executing capability"
            " intent=" + result.intentName
            + ", capability=" + capabilityName
            + ", args=" + summarizeJsonForLog(capabilityArgs));
        result.capabilityResult = intent->runCapabilitySync(capabilityName, capabilityArgs, 4000);
        CubeLog::info(
            "DecisionEngine: capability result"
            " intent=" + result.intentName
            + ", capability=" + capabilityName
            + ", result=" + summarizeJsonForLog(result.capabilityResult));
        applyCapabilityResultToIntent(intent, result.capabilityResult);
        if (result.capabilityResult.is_object() && result.capabilityResult.contains("error")) {
            CubeLog::warning(
                "DecisionEngine: capability reported error"
                " intent=" + result.intentName
                + ", capability=" + capabilityName
                + ", error=" + jsonToParameterString(result.capabilityResult["error"]));
            result.executionStatus = "capability_error";
            result.error = jsonToParameterString(result.capabilityResult["error"]);
            result.responseText = intent->getResponseString().empty()
                ? "I found the right action, but it failed."
                : intent->getResponseString();
            return result;
        }
    }

    if (result.intentName == "core.ask_ai") {
        result.speakResult = shouldSpeakForGeneralAiAnswer();
    }

    intent->execute();
    result.responseText = intent->getResponseString();
    if (result.responseText.empty()) {
        result.responseText = "Done.";
    }
    result.executionStatus = "success";
    CubeLog::info(
        "DecisionEngine: intent execution complete"
        " intent=" + result.intentName
        + ", status=" + result.executionStatus
        + ", response=" + result.responseText);
    return result;
}

DecisionTurnResult DecisionEngineMain::processTranscript(const std::string& transcript)
{
    CubeLog::info("DecisionEngine: processing transcript: " + transcript);
    if (!remoteServerAPI || !intentRegistry || !functionRegistry) {
        return DecisionTurnResult {
            .transcript = transcript,
            .intentName = "",
            .executionStatus = "error",
            .responseText = "",
            .capabilityResult = nlohmann::json::object(),
            .error = "intent_resolution_unavailable",
            .timestampEpochMs = nowEpochMs()
        };
    }

    const auto tools = buildVoiceToolCatalogue(intentRegistry, functionRegistry);
    const auto resolved = remoteServerAPI->getResolvedIntentCallAsync(
        transcript,
        tools,
        nlohmann::json({
            { "deviceTimezone", localTimezoneId() },
            { "deviceNowEpochMs", nowEpochMs() },
            { "deviceNowIsoLocal", nowIsoLocal() }
        }))
                              .get();

    CubeLog::info(
        "DecisionEngine: resolved intent_call"
        " status=" + resolved.status
        + ", intentName=" + resolved.intentName
        + ", arguments=" + summarizeJsonForLog(resolved.arguments)
        + ", message=" + resolved.message);

    if (resolved.status == "matched") {
        auto intent = intentRegistry->getIntent(resolved.intentName);
        if (!intent) {
            CubeLog::warning("DecisionEngine: resolved intent not registered locally: " + resolved.intentName);
            return DecisionTurnResult {
                .transcript = transcript,
                .intentName = resolved.intentName,
                .executionStatus = "intent_not_found",
                .responseText = "I found a matching action, but it isn't available on this device.",
                .capabilityResult = nlohmann::json::object(),
                .error = "intent_not_found",
                .timestampEpochMs = nowEpochMs()
            };
        }
        return executeIntent(intent, transcript, resolved.arguments);
    }

    if (resolved.status == "error") {
        CubeLog::warning("DecisionEngine: intent resolution returned error: " + resolved.message);
        return DecisionTurnResult {
            .transcript = transcript,
            .intentName = "",
            .executionStatus = "error",
            .responseText = "I couldn't process that request right now.",
            .capabilityResult = nlohmann::json::object(),
            .error = resolved.message.empty() ? "intent_resolution_failed" : resolved.message,
            .timestampEpochMs = nowEpochMs()
        };
    }

    if (shouldFallbackToAskAi(transcript) && intentRegistry) {
        CubeLog::info("DecisionEngine: falling back to core.ask_ai for general question");
        auto intent = intentRegistry->getIntent("core.ask_ai");
        return executeIntent(intent, transcript);
    }

    CubeLog::warning("DecisionEngine: no intent matched transcript and no ask_ai fallback applied: " + transcript);
    return executeIntent(nullptr, transcript);
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
        { "turnState", turnStateName(turnState) },
        { "latestTranscriptEvent", latestTranscriptEvent },
        { "activeTranscriptPreview", activeTranscriptPreview },
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
