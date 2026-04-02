/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "decisions.h"
#include "../audio/audioOutput.h"
#include "../database/cubeDB.h"
#include "../gui/gui.h"
#include "nlohmann/json-schema.hpp"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <cctype>
#include <algorithm>
#include <chrono>
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

bool isPersonalityRewriteEnabled()
{
    try {
        const auto setting = GlobalSettings::getSetting(GlobalSettings::SettingType::PERSONALITY_ENABLED);
        if (setting.is_boolean()) {
            return setting.get<bool>();
        }
    } catch (const std::exception& e) {
        CubeLog::warning(std::string("DecisionEngine: failed to read personality setting: ") + e.what());
    } catch (...) {
        CubeLog::warning("DecisionEngine: failed to read personality setting");
    }
    return true;
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

std::string formatDurationMsOrNa(int64_t startEpochMs, int64_t endEpochMs)
{
    if (startEpochMs <= 0 || endEpochMs <= 0 || endEpochMs < startEpochMs) {
        return "n/a";
    }
    return std::to_string(endEpochMs - startEpochMs);
}

std::vector<uint32_t> decodeUtf8ToCodepoints(const std::string& text)
{
    std::vector<uint32_t> codepoints;
    codepoints.reserve(text.size());

    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    size_t index = 0;
    while (index < text.size()) {
        const unsigned char lead = bytes[index];
        if (lead < 0x80) {
            codepoints.push_back(static_cast<uint32_t>(lead));
            ++index;
            continue;
        }

        uint32_t codepoint = 0;
        size_t sequenceLength = 0;
        uint32_t minValue = 0;
        if ((lead & 0xE0) == 0xC0) {
            codepoint = lead & 0x1F;
            sequenceLength = 2;
            minValue = 0x80;
        } else if ((lead & 0xF0) == 0xE0) {
            codepoint = lead & 0x0F;
            sequenceLength = 3;
            minValue = 0x800;
        } else if ((lead & 0xF8) == 0xF0) {
            codepoint = lead & 0x07;
            sequenceLength = 4;
            minValue = 0x10000;
        } else {
            ++index;
            continue;
        }

        if (index + sequenceLength > text.size()) {
            ++index;
            continue;
        }

        bool valid = true;
        for (size_t offset = 1; offset < sequenceLength; ++offset) {
            const unsigned char continuation = bytes[index + offset];
            if ((continuation & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            codepoint = (codepoint << 6) | static_cast<uint32_t>(continuation & 0x3F);
        }

        if (!valid
            || codepoint < minValue
            || codepoint > 0x10FFFF
            || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            ++index;
            continue;
        }

        codepoints.push_back(codepoint);
        index += sequenceLength;
    }

    return codepoints;
}

void appendUtf8Codepoint(std::string& out, uint32_t codepoint)
{
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
        return;
    }
    if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        return;
    }
    if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        return;
    }
    out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
}

bool isEmojiLikeCodepoint(uint32_t codepoint)
{
    return (codepoint >= 0x1F000 && codepoint <= 0x1FAFF)
        || (codepoint >= 0x2300 && codepoint <= 0x23FF)
        || (codepoint >= 0x2600 && codepoint <= 0x27BF)
        || codepoint == 0x00A9
        || codepoint == 0x00AE
        || codepoint == 0x203C
        || codepoint == 0x2049
        || codepoint == 0x2122
        || codepoint == 0x2139;
}

std::string stripEmojiLikeCodepoints(const std::string& input)
{
    auto codepoints = decodeUtf8ToCodepoints(input);
    std::string out;
    out.reserve(input.size());
    for (uint32_t codepoint : codepoints) {
        if (isEmojiLikeCodepoint(codepoint)
            || codepoint == 0xFE0E
            || codepoint == 0xFE0F
            || codepoint == 0x200D
            || (codepoint >= 0x1F3FB && codepoint <= 0x1F3FF)) {
            continue;
        }
        appendUtf8Codepoint(out, codepoint);
    }
    return out;
}

std::chrono::milliseconds resultPresentationDuration()
{
    try {
        const auto raw = Config::get("DECISION_ENGINE_RESULT_HIDE_MS", "4000");
        const auto value = std::max<long long>(0, std::stoll(raw));
        return std::chrono::milliseconds(value);
    } catch (...) {
        return std::chrono::milliseconds(4000);
    }
}

std::string voiceFailureCategoryName(TheCubeServer::TheCubeServerAPI::VoiceFailureCategory category)
{
    switch (category) {
    case TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::NONE:
        return "none";
    case TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE:
        return "voice_service_unavailable";
    case TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::OTHER_VOICE_FAILURE:
        return "other_voice_failure";
    }
    return "unknown";
}

std::string valueOrNone(const std::string& value)
{
    return value.empty() ? "<none>" : value;
}

nlohmann::json jsonValueFromString(const std::string& value)
{
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }
    if (lower == "true")
        return true;
    if (lower == "false")
        return false;

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

nlohmann::json emotionsToJson(const std::vector<Personality::EmotionSimple>& emotions)
{
    nlohmann::json state = nlohmann::json::object();
    for (const auto& emotion : emotions) {
        state[Personality::emotionToString(emotion.emotion)] = emotion.value;
    }
    return state;
}

struct EmotionalStyleProfile {
    std::string dominantEmotion;
    int dominantValue = 0;
    std::string secondaryEmotion;
    int secondaryValue = 0;
    std::string intensityBucket = "low";
};

EmotionalStyleProfile buildEmotionalStyleProfile(const std::vector<Personality::EmotionSimple>& emotions)
{
    EmotionalStyleProfile profile;
    if (emotions.empty()) {
        return profile;
    }

    std::vector<Personality::EmotionSimple> sorted = emotions;
    std::sort(sorted.begin(), sorted.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.value > rhs.value;
    });

    profile.dominantEmotion = Personality::emotionToString(sorted.front().emotion);
    profile.dominantValue = sorted.front().value;
    if (sorted.size() > 1) {
        profile.secondaryEmotion = Personality::emotionToString(sorted[1].emotion);
        profile.secondaryValue = sorted[1].value;
    }

    if (profile.dominantValue >= 85) {
        profile.intensityBucket = "very_high";
    } else if (profile.dominantValue >= 70) {
        profile.intensityBucket = "high";
    } else if (profile.dominantValue >= 45) {
        profile.intensityBucket = "medium";
    } else {
        profile.intensityBucket = "low";
    }

    return profile;
}

std::string responseCategoryForTurnResult(const DecisionTurnResult& result)
{
    if (!result.error.empty()
        || result.executionStatus == "error"
        || result.executionStatus == "capability_error"
        || result.executionStatus == "capability_validation_error"
        || result.executionStatus == "voice_service_unavailable"
        || result.executionStatus == "transcription_failed") {
        return "error";
    }
    if (result.intentName == "core.ask_ai") {
        return "general_answer";
    }
    if (result.intentName.rfind("core.get_", 0) == 0) {
        return "utility_fact";
    }
    return "confirmation";
}

bool shouldPersistChatHistory(const DecisionTurnResult& result)
{
    return result.executionStatus == "success"
        && !result.historyKey.empty()
        && !result.transcript.empty();
}

nlohmann::json recentToolHistoryToJson(const std::vector<ChatHistoryEntry>& entries)
{
    auto history = nlohmann::json::array();
    for (const auto& entry : entries) {
        history.push_back({
            { "requestText", entry.requestText },
            { "displayResponse", entry.displayResponse },
            { "createdAtMs", entry.createdAtMs }
        });
    }
    return history;
}

std::string jsonToParameterString(const nlohmann::json& value)
{
    if (value.is_string())
        return value.get<std::string>();
    if (value.is_boolean())
        return value.get<bool>() ? "true" : "false";
    if (value.is_number_integer())
        return std::to_string(value.get<long long>());
    if (value.is_number_float())
        return std::to_string(value.get<double>());
    return value.dump();
}

std::string lookupParameter(const Parameters& parameters, const std::string& key)
{
    const auto it = parameters.find(key);
    if (it == parameters.end())
        return {};
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
        tools.push_back({ { "name", makeOpenAiToolName(intent->getIntentName()) },
            { "intentName", intent->getIntentName() },
            { "description", intent->getBriefDesc() },
            { "parameters", effectiveVoiceSchema(intent, capability) } });
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

nlohmann::json parametersToJson(const Parameters& parameters)
{
    nlohmann::json args = nlohmann::json::object();
    for (const auto& [key, value] : parameters) {
        if (key == "capability_name")
            continue;
        args[key] = jsonValueFromString(value);
    }
    return args;
}

nlohmann::json buildCapabilityArgs(
    const Parameters& parameters,
    const nlohmann::json& resolvedArgs,
    const std::string& transcript)
{
    auto capabilityArgs = parametersToJson(parameters);
    if (resolvedArgs.is_object()) {
        for (const auto& [key, value] : resolvedArgs.items()) {
            capabilityArgs[key] = value;
        }
    }
    if (!capabilityArgs.contains("transcript")) {
        capabilityArgs["transcript"] = transcript;
    }
    return capabilityArgs;
}

void applyCapabilityResultToIntent(const std::shared_ptr<Intent>& intent, const nlohmann::json& capabilityResult)
{
    if (!intent || !capabilityResult.is_object()) {
        return;
    }

    for (const auto& [key, value] : capabilityResult.items()) {
        intent->addParameter(key, jsonToParameterString(value));
    }
}

DecisionTurnResult makeIntentResolutionUnavailableResult(const std::string& transcript)
{
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

DecisionTurnResult makeResolvedIntentNotFoundResult(const std::string& transcript, const std::string& intentName)
{
    return DecisionTurnResult {
        .transcript = transcript,
        .intentName = intentName,
        .executionStatus = "intent_not_found",
        .responseText = "I found a matching action, but it isn't available on this device.",
        .capabilityResult = nlohmann::json::object(),
        .error = "intent_not_found",
        .timestampEpochMs = nowEpochMs()
    };
}

DecisionTurnResult makeIntentResolutionErrorResult(const std::string& transcript, const std::string& message)
{
    return DecisionTurnResult {
        .transcript = transcript,
        .intentName = "",
        .executionStatus = "error",
        .responseText = "I couldn't process that request right now.",
        .capabilityResult = nlohmann::json::object(),
        .error = message.empty() ? "intent_resolution_failed" : message,
        .timestampEpochMs = nowEpochMs()
    };
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
        {
            std::scoped_lock lock(mutex_);
            liveTranscript_.clear();
            hasVisibleTranscript_ = false;
        }
        GUI::showMessageBox("TheCube", "Listening...");
    }

    void updateTranscript(const TranscriptionEvent& event)
    {
        if (event.isFinal) {
            if (event.fullText.empty()) {
                return;
            }
            std::scoped_lock lock(mutex_);
            liveTranscript_ = event.fullText;
            hasVisibleTranscript_ = true;
            GUI::showMessageBox("TheCube", liveTranscript_);
            return;
        }

        if (event.appendText.empty()) {
            return;
        }

        std::scoped_lock lock(mutex_);
        if (!hasVisibleTranscript_) {
            liveTranscript_ = event.appendText;
            hasVisibleTranscript_ = !liveTranscript_.empty();
        } else {
            liveTranscript_ += event.appendText;
        }
        if (!liveTranscript_.empty()) {
            GUI::showMessageBox("TheCube", liveTranscript_);
        }
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
    std::mutex mutex_;
    std::string liveTranscript_;
    bool hasVisibleTranscript_ = false;
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
        try {
            intentRegistry->registerInterface();
        } catch (...) {
            CubeLog::error("Failed to register IntentRegistry interface");
        }
    }
    if (functionRegistry) {
        functionRegistry->setRemoteConversationClient(remoteServerAPI);
        functionRegistry->setNotificationCenter(notificationCenter);
        try {
            functionRegistry->registerInterface();
        } catch (...) {
            CubeLog::error("Failed to register FunctionRegistry interface");
        }
    }
    if (scheduler) {
        scheduler->setIntentRegistry(intentRegistry);
        scheduler->setFunctionRegistry(functionRegistry);
        try {
            scheduler->registerInterface();
        } catch (...) {
            CubeLog::error("Failed to register Scheduler interface");
        }
    }
    if (notificationCenter) {
        notificationCenter->setScheduler(scheduler);
        NotificationCenter::setSharedInstance(notificationCenter);
        try {
            notificationCenter->registerInterface();
        } catch (...) {
            CubeLog::error("Failed to register NotificationCenter interface");
        }
    }
    if (triggerManager) {
        triggerManager->setIntentRegistry(intentRegistry);
        triggerManager->setFunctionRegistry(functionRegistry);
        try {
            triggerManager->registerInterface();
        } catch (...) {
            CubeLog::error("Failed to register TriggerManager interface");
        }
    }
    if (personalityManager) {
        try {
            personalityManager->registerInterface();
        } catch (...) {
            CubeLog::error("Failed to register PersonalityManager interface");
        }
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

    try {
        auto dbManager = CubeDB::getDBManager();
        if (dbManager) {
            chatHistoryStore = std::make_shared<ChatHistoryStore>(dbManager);
        }
    } catch (...) {
    }

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
    transcriptionEventHandle = TranscriptionEvents::subscribe([this](const TranscriptionEvent& event) {
        handleTranscriptEvent(event);
    });

    if (scheduler)
        scheduler->start();
    if (notificationCenter)
        notificationCenter->start();
    if (triggerManager)
        triggerManager->start();
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
    if (notificationCenter)
        notificationCenter->stop();
    if (scheduler)
        scheduler->stop();
    if (triggerManager)
        triggerManager->stop();
}

void DecisionEngineMain::restart()
{
    stop();
    start();
}

void DecisionEngineMain::pause()
{
    if (scheduler)
        scheduler->pause();
}

void DecisionEngineMain::resume()
{
    if (scheduler)
        scheduler->resume();
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

void DecisionEngineMain::maybeCaptureVoiceSessionIdLocked()
{
    if (!currentTurnTiming.voiceSessionId.empty() || !remoteServerAPI) {
        return;
    }

    const auto session = remoteServerAPI->getActiveTranscriptionSession();
    if (session.has_value() && !session->sessionId.empty()) {
        currentTurnTiming.voiceSessionId = session->sessionId;
    }
}

void DecisionEngineMain::logTurnTimingStage(
    const TurnTimingMetrics& snapshot,
    const std::string& stage,
    int64_t stageEpochMs,
    const std::string& extra) const
{
    if (snapshot.turnId == 0 || stageEpochMs <= 0) {
        return;
    }

    std::string message =
        "DecisionEngine: turn_timing"
        " stage=" + stage
        + ", turnId=" + std::to_string(snapshot.turnId)
        + ", session=" + valueOrNone(snapshot.voiceSessionId)
        + ", tsEpochMs=" + std::to_string(stageEpochMs)
        + ", sinceWakeMs=" + formatDurationMsOrNa(snapshot.wakeDetectedEpochMs, stageEpochMs);
    if (!extra.empty()) {
        message += ", " + extra;
    }
    CubeLog::info(message);
}

void DecisionEngineMain::beginTurnTiming()
{
    const auto stageEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    {
        std::scoped_lock lock(stateMutex);
        currentTurnTiming = {};
        currentTurnTiming.turnId = nextTurnId++;
        currentTurnTiming.wakeDetectedEpochMs = stageEpochMs;
        maybeCaptureVoiceSessionIdLocked();
        snapshot = currentTurnTiming;
    }
    logTurnTimingStage(snapshot, "wake_detected", stageEpochMs);
}

void DecisionEngineMain::noteListeningUiShown()
{
    const auto stageEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    {
        std::scoped_lock lock(stateMutex);
        if (currentTurnTiming.turnId == 0) {
            return;
        }
        currentTurnTiming.listeningUiShownEpochMs = stageEpochMs;
        maybeCaptureVoiceSessionIdLocked();
        snapshot = currentTurnTiming;
    }
    logTurnTimingStage(snapshot, "listening_ui_shown", stageEpochMs);
}

void DecisionEngineMain::noteFirstPartial(const TranscriptionEvent& event)
{
    const auto stageEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    bool shouldLog = false;
    {
        std::scoped_lock lock(stateMutex);
        if (currentTurnTiming.turnId == 0) {
            return;
        }
        ++currentTurnTiming.partialEventCount;
        if (!event.fullText.empty()) {
            currentTurnTiming.lastTranscriptLength = event.fullText.size();
        } else if (!event.appendText.empty()) {
            currentTurnTiming.lastTranscriptLength += event.appendText.size();
        }
        if (currentTurnTiming.firstPartialEpochMs == 0) {
            currentTurnTiming.firstPartialEpochMs = stageEpochMs;
            shouldLog = true;
        }
        maybeCaptureVoiceSessionIdLocked();
        snapshot = currentTurnTiming;
    }
    if (!shouldLog) {
        return;
    }
    logTurnTimingStage(snapshot,
        "first_partial",
        stageEpochMs,
        "appendLength=" + std::to_string(event.appendText.size())
            + ", fullLength=" + std::to_string(event.fullText.size()));
}

void DecisionEngineMain::noteFinalTranscript(const std::string& transcript)
{
    const auto stageEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    {
        std::scoped_lock lock(stateMutex);
        if (currentTurnTiming.turnId == 0) {
            return;
        }
        currentTurnTiming.finalTranscriptEpochMs = stageEpochMs;
        currentTurnTiming.lastTranscriptLength = transcript.size();
        maybeCaptureVoiceSessionIdLocked();
        snapshot = currentTurnTiming;
    }
    logTurnTimingStage(snapshot,
        "final_transcript",
        stageEpochMs,
        "transcriptLength=" + std::to_string(transcript.size())
            + ", partialCount=" + std::to_string(snapshot.partialEventCount));
}

void DecisionEngineMain::noteIntentResolved(const TheCubeServer::ResolvedIntentCall& resolved)
{
    const auto stageEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    {
        std::scoped_lock lock(stateMutex);
        if (currentTurnTiming.turnId == 0) {
            return;
        }
        currentTurnTiming.intentResolvedEpochMs = stageEpochMs;
        maybeCaptureVoiceSessionIdLocked();
        snapshot = currentTurnTiming;
    }
    logTurnTimingStage(snapshot,
        "intent_resolved",
        stageEpochMs,
        "status=" + resolved.status
            + ", intentName=" + valueOrNone(resolved.intentName));
}

void DecisionEngineMain::noteResultPresented(const DecisionTurnResult& result, const std::string& displayMessage)
{
    const auto stageEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    {
        std::scoped_lock lock(stateMutex);
        if (currentTurnTiming.turnId == 0) {
            return;
        }
        currentTurnTiming.resultPresentedEpochMs = stageEpochMs;
        maybeCaptureVoiceSessionIdLocked();
        snapshot = currentTurnTiming;
    }
    logTurnTimingStage(snapshot,
        "result_presented",
        stageEpochMs,
        "status=" + result.executionStatus
            + ", intentName=" + valueOrNone(result.intentName)
            + ", displayLength=" + std::to_string(displayMessage.size()));
}

void DecisionEngineMain::noteSpeechRequested()
{
    const auto stageEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    {
        std::scoped_lock lock(stateMutex);
        if (currentTurnTiming.turnId == 0) {
            return;
        }
        currentTurnTiming.speechRequestedEpochMs = stageEpochMs;
        maybeCaptureVoiceSessionIdLocked();
        snapshot = currentTurnTiming;
    }
    logTurnTimingStage(snapshot, "speech_requested", stageEpochMs);
}

void DecisionEngineMain::finalizeTurnTiming(const std::string& reason, const DecisionTurnResult* result)
{
    const auto summaryEpochMs = nowEpochMs();
    TurnTimingMetrics snapshot;
    {
        std::scoped_lock lock(stateMutex);
        if (currentTurnTiming.turnId == 0 || currentTurnTiming.summaryLogged) {
            return;
        }
        maybeCaptureVoiceSessionIdLocked();
        currentTurnTiming.summaryLogged = true;
        snapshot = currentTurnTiming;
    }

    std::string message =
        "DecisionEngine: turn_timing_summary"
        " turnId=" + std::to_string(snapshot.turnId)
        + ", session=" + valueOrNone(snapshot.voiceSessionId)
        + ", reason=" + reason
        + ", wakeTs=" + std::to_string(snapshot.wakeDetectedEpochMs)
        + ", listeningTs=" + std::to_string(snapshot.listeningUiShownEpochMs)
        + ", firstPartialTs=" + std::to_string(snapshot.firstPartialEpochMs)
        + ", finalTranscriptTs=" + std::to_string(snapshot.finalTranscriptEpochMs)
        + ", intentResolvedTs=" + std::to_string(snapshot.intentResolvedEpochMs)
        + ", resultPresentedTs=" + std::to_string(snapshot.resultPresentedEpochMs)
        + ", speechRequestedTs=" + std::to_string(snapshot.speechRequestedEpochMs)
        + ", wakeToListeningMs=" + formatDurationMsOrNa(snapshot.wakeDetectedEpochMs, snapshot.listeningUiShownEpochMs)
        + ", wakeToFirstPartialMs=" + formatDurationMsOrNa(snapshot.wakeDetectedEpochMs, snapshot.firstPartialEpochMs)
        + ", wakeToFinalTranscriptMs=" + formatDurationMsOrNa(snapshot.wakeDetectedEpochMs, snapshot.finalTranscriptEpochMs)
        + ", finalTranscriptToIntentResolvedMs=" + formatDurationMsOrNa(snapshot.finalTranscriptEpochMs, snapshot.intentResolvedEpochMs)
        + ", intentResolvedToResultPresentedMs=" + formatDurationMsOrNa(snapshot.intentResolvedEpochMs, snapshot.resultPresentedEpochMs)
        + ", resultPresentedToSpeechRequestedMs=" + formatDurationMsOrNa(snapshot.resultPresentedEpochMs, snapshot.speechRequestedEpochMs)
        + ", wakeToSummaryMs=" + formatDurationMsOrNa(snapshot.wakeDetectedEpochMs, summaryEpochMs)
        + ", partialCount=" + std::to_string(snapshot.partialEventCount)
        + ", transcriptLength=" + std::to_string(snapshot.lastTranscriptLength);

    if (result) {
        message += ", status=" + result->executionStatus
            + ", intentName=" + valueOrNone(result->intentName)
            + ", error=" + valueOrNone(result->error);
    }

    CubeLog::info(message);
}

void DecisionEngineMain::hideTurnUi()
{
    presentationController().hide();
}

void DecisionEngineMain::showListeningUi()
{
    presentationController().showListening();
}

DecisionTurnResult DecisionEngineMain::makeVoiceServiceUnavailableResult(const std::string& transcript) const
{
    return DecisionTurnResult {
        .transcript = transcript,
        .intentName = "",
        .executionStatus = "voice_service_unavailable",
        .responseText = "Voice service is unavailable right now. Please try again shortly.",
        .capabilityResult = nlohmann::json::object(),
        .error = "voice_service_unavailable",
        .speakResult = false,
        .timestampEpochMs = nowEpochMs()
    };
}

bool DecisionEngineMain::hasRemoteVoiceServiceFailure() const
{
    return remoteServerAPI
        && remoteServerAPI->getLastVoiceFailureCategory()
            == TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE;
}

void DecisionEngineMain::scheduleReturnToIdle(std::chrono::milliseconds delay)
{
    const auto generation = resultPresentationGeneration.fetch_add(1, std::memory_order_relaxed) + 1;
    std::thread([this, delay, generation]() {
        std::this_thread::sleep_for(delay);
        if (resultPresentationGeneration.load(std::memory_order_relaxed) != generation) {
            return;
        }
        std::scoped_lock lock(stateMutex);
        if (turnState == TurnState::RESULT_PRESENTED) {
            turnState = TurnState::IDLE;
        }
    }).detach();
}

void DecisionEngineMain::handleTranscriptEvent(const TranscriptionEvent& event)
{
    if (event.fullText.empty() && event.appendText.empty()) {
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
        if (!event.fullText.empty()) {
            latestTranscriptEvent = event.fullText;
        }
        if (event.isFinal) {
            activeTranscriptPreview = event.fullText;
            turnState = TurnState::FINAL_TRANSCRIPT_PENDING_INTENT;
        } else {
            if (!event.appendText.empty()) {
                activeTranscriptPreview += event.appendText;
            }
            turnState = TurnState::TRANSCRIPT_STREAMING;
        }
    }

    if (event.isFinal) {
        noteFinalTranscript(event.fullText);
    } else {
        noteFirstPartial(event);
    }

    presentationController().updateTranscript(event);
}

void DecisionEngineMain::presentTurnResult(const DecisionTurnResult& result)
{
    const auto hideDelay = resultPresentationDuration();
    const std::string sourceMessage = !result.responseText.empty()
        ? result.responseText
        : (!result.error.empty() ? result.error : "Done.");
    std::string message = stripEmojiLikeCodepoints(sourceMessage);
    const bool remoteVoiceUnavailable = hasRemoteVoiceServiceFailure();
    nlohmann::json rewriteContext = nlohmann::json::object();
    if (chatHistoryStore && shouldPersistChatHistory(result)) {
        const auto recentHistory = chatHistoryStore->getRecentHistory(result.historyKey, 10);
        rewriteContext["historyKey"] = result.historyKey;
        rewriteContext["recentToolHistory"] = recentToolHistoryToJson(recentHistory);
    }
    if (personalityManager
        && remoteServerAPI
        && isPersonalityRewriteEnabled()
        && !message.empty()
        && result.error != "voice_service_unavailable"
        && !remoteVoiceUnavailable) {
        const auto emotions = personalityManager->getAllEmotionsCurrent();
        const auto responseCategory = responseCategoryForTurnResult(result);
        auto rewriteFuture = modifyStringUsingAIForEmotionalState(
            message,
            emotions,
            remoteServerAPI,
            responseCategory,
            rewriteContext);
        using namespace std::chrono_literals;
        if (rewriteFuture.wait_for(2500ms) == std::future_status::ready) {
            try {
                const auto rewritten = rewriteFuture.get();
                if (!rewritten.empty()) {
                    message = stripEmojiLikeCodepoints(rewritten);
                }
            } catch (const std::exception& e) {
                CubeLog::warning(
                    std::string("DecisionEngine: emotional rewrite failed: ")
                    + e.what());
            } catch (...) {
                CubeLog::warning("DecisionEngine: emotional rewrite failed with unknown error");
            }
        } else {
            CubeLog::warning("DecisionEngine: emotional rewrite timed out; using original response");
        }
    }
    CubeLog::info(
        "DecisionEngine: presenting result"
        " status="
        + result.executionStatus
        + ", intent=" + result.intentName
        + ", error=" + result.error
        + ", sourceMessage=" + sourceMessage
        + ", displayMessage=" + message);
    presentationController().showResult(message);
    noteResultPresented(result, message);

    if (chatHistoryStore && shouldPersistChatHistory(result)) {
        const bool stored = chatHistoryStore->appendHistory(ChatHistoryEntry {
            .createdAtMs = result.timestampEpochMs,
            .historyKey = result.historyKey,
            .intentName = result.intentName,
            .capabilityName = result.capabilityName,
            .requestText = result.transcript,
            .displayResponse = message
        });
        if (!stored) {
            CubeLog::warning("DecisionEngine: failed to persist chat history for " + result.historyKey);
        }
    }

    if (result.speakResult && !message.empty() && functionRegistry) {
        noteSpeechRequested();
        functionRegistry->runCapabilityAsync(
            "core.speak_text",
            nlohmann::json({ { "text", message } }),
            nullptr);
    }

    setTurnState(TurnState::RESULT_PRESENTED);
    presentationController().scheduleHide(hideDelay);
    scheduleReturnToIdle(hideDelay);
    finalizeTurnTiming("result_presented", &result);
}

void DecisionEngineMain::onWakeWordDetected()
{
    CubeLog::info("DecisionEngine: wake word detected");
    finalizeTurnTiming("interrupted_by_new_wake");
    beginTurnTiming();
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
    if (remoteServerAPI
        && (remoteServerAPI->getServerStatus() == TheCubeServer::TheCubeServerAPI::ServerStatus::SERVER_STATUS_ERROR
            || remoteServerAPI->getLastVoiceFailureCategory() != TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::NONE)) {
        remoteServerAPI->resetServerConnection();
    }
    if (remoteServerAPI && intentRegistry && functionRegistry) {
        remoteServerAPI->prepareVoiceTurn(
            buildVoiceToolCatalogue(intentRegistry, functionRegistry),
            nlohmann::json({ { "deviceTimezone", localTimezoneId() },
                { "deviceNowEpochMs", nowEpochMs() },
                { "deviceNowIsoLocal", nowIsoLocal() } }));
    }
    transcription = transcriber ? transcriber->transcribeQueue(audioQueue) : nullptr;
    if (!transcription) {
        CubeLog::error("DecisionEngine: transcription queue was not created");
        if (hasRemoteVoiceServiceFailure()) {
            const auto failedTurn = makeVoiceServiceUnavailableResult("");
            recordTurnResult(failedTurn);
            presentTurnResult(failedTurn);
        } else {
            hideTurnUi();
            setTurnState(TurnState::IDLE);
            finalizeTurnTiming("transcription_queue_creation_failed");
        }
        return;
    }
    showListeningUi();
    CubeLog::info("DecisionEngine: listening UI shown");
    noteListeningUiShown();
    setTurnState(TurnState::LISTENING);

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
                if (hasRemoteVoiceServiceFailure()) {
                    const auto failedTurn = makeVoiceServiceUnavailableResult("");
                    recordTurnResult(failedTurn);
                    presentTurnResult(failedTurn);
                    break;
                }
                const DecisionTurnResult failedTurn {
                    .transcript = "",
                    .intentName = "",
                    .executionStatus = "transcription_failed",
                    .responseText = "",
                    .capabilityResult = nlohmann::json::object(),
                    .error = "transcription_failed",
                    .speakResult = false,
                    .timestampEpochMs = nowEpochMs() };
                recordTurnResult(failedTurn);
                hideTurnUi();
                setTurnState(TurnState::IDLE);
                finalizeTurnTiming("transcription_failed", &failedTurn);
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
    result.capabilityName = capabilityName;
    result.historyKey = !capabilityName.empty() ? capabilityName : result.intentName;
    result.speakResult = parseBoolString(lookupParameter(parameters, "speak_result"));

    if (!capabilityName.empty()) {
        if (const auto* capability = functionRegistry ? functionRegistry->findCapability(capabilityName) : nullptr) {
            const auto schema = effectiveVoiceSchema(intent, capability);
            std::string validationError;
            const auto argsToValidate = resolvedArgs.is_object() ? resolvedArgs : nlohmann::json::object();
            if (!validateJsonAgainstSchema(argsToValidate, schema, validationError)) {
                CubeLog::warning(
                    "DecisionEngine: capability argument validation failed"
                    " intent="
                    + result.intentName
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

        auto capabilityArgs = buildCapabilityArgs(parameters, resolvedArgs, transcript);

        CubeLog::info(
            "DecisionEngine: executing capability"
            " intent="
            + result.intentName
            + ", capability=" + capabilityName
            + ", args=" + summarizeJsonForLog(capabilityArgs));
        result.capabilityResult = intent->runCapabilitySync(capabilityName, capabilityArgs, 4000);
        CubeLog::info(
            "DecisionEngine: capability result"
            " intent="
            + result.intentName
            + ", capability=" + capabilityName
            + ", result=" + summarizeJsonForLog(result.capabilityResult));
        applyCapabilityResultToIntent(intent, result.capabilityResult);
        if (result.capabilityResult.is_object() && result.capabilityResult.contains("error")) {
            CubeLog::warning(
                "DecisionEngine: capability reported error"
                " intent="
                + result.intentName
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
        " intent="
        + result.intentName
        + ", status=" + result.executionStatus
        + ", response=" + result.responseText);
    return result;
}

DecisionTurnResult DecisionEngineMain::processTranscript(const std::string& transcript)
{
    CubeLog::info("DecisionEngine: processing transcript: " + transcript);
    const auto cleanupVoiceTurn = [this]() {
        if (remoteServerAPI) {
            remoteServerAPI->cancelStreamingTranscription();
        }
    };
    if (!remoteServerAPI || !intentRegistry || !functionRegistry) {
        auto result = makeIntentResolutionUnavailableResult(transcript);
        cleanupVoiceTurn();
        return result;
    }

    const auto resolved = remoteServerAPI->waitForResolvedIntentCall(std::chrono::milliseconds(15000));
    noteIntentResolved(resolved);

    CubeLog::info(
        "DecisionEngine: resolved intent_call"
        " status="
        + resolved.status
        + ", intentName=" + resolved.intentName
        + ", arguments=" + summarizeJsonForLog(resolved.arguments)
        + ", message=" + resolved.message);

    if (resolved.status == "matched") {
        auto intent = intentRegistry->getIntent(resolved.intentName);
        if (!intent) {
            CubeLog::warning("DecisionEngine: resolved intent not registered locally: " + resolved.intentName);
            auto result = makeResolvedIntentNotFoundResult(transcript, resolved.intentName);
            cleanupVoiceTurn();
            return result;
        }
        auto result = executeIntent(intent, transcript, resolved.arguments);
        cleanupVoiceTurn();
        return result;
    }

    if (resolved.status == "error") {
        CubeLog::warning("DecisionEngine: intent resolution returned error: " + resolved.message);
        auto result = hasRemoteVoiceServiceFailure()
            ? makeVoiceServiceUnavailableResult(transcript)
            : makeIntentResolutionErrorResult(transcript, resolved.message);
        cleanupVoiceTurn();
        return result;
    }

    if (shouldFallbackToAskAi(transcript) && intentRegistry) {
        CubeLog::info("DecisionEngine: falling back to core.ask_ai for general question");
        auto intent = intentRegistry->getIntent("core.ask_ai");
        auto result = executeIntent(intent, transcript);
        cleanupVoiceTurn();
        return result;
    }

    CubeLog::warning("DecisionEngine: no intent matched transcript and no ask_ai fallback applied: " + transcript);
    auto result = executeIntent(nullptr, transcript);
    cleanupVoiceTurn();
    return result;
}

void DecisionEngineMain::recordTurnResult(const DecisionTurnResult& result)
{
    std::scoped_lock lk(stateMutex);
    lastDecisionResult = result;
}

nlohmann::json DecisionEngineMain::statusJson() const
{
    std::scoped_lock lk(stateMutex);
    return nlohmann::json({ { "started", started },
        { "turnState", turnStateName(turnState) },
        { "latestTranscriptEvent", latestTranscriptEvent },
        { "activeTranscriptPreview", activeTranscriptPreview },
        { "remoteServerStatus", static_cast<int>(remoteServerAPI ? remoteServerAPI->getServerStatus() : TheCubeServer::TheCubeServerAPI::ServerStatus::SERVER_STATUS_ERROR) },
        { "remoteServerError", static_cast<int>(remoteServerAPI ? remoteServerAPI->getServerError() : TheCubeServer::TheCubeServerAPI::ServerError::SERVER_ERROR_INTERNAL_ERROR) },
        { "remoteVoiceFailureCategory", remoteServerAPI ? voiceFailureCategoryName(remoteServerAPI->getLastVoiceFailureCategory()) : "other_voice_failure" },
        { "remoteVoiceFailureMessage", remoteServerAPI ? remoteServerAPI->getLastVoiceFailureMessage() : "decision_engine_remote_server_unavailable" },
        { "lastDecisionResult", lastDecisionResult.toJson() } });
}

HttpEndPointData_t DecisionEngineMain::getHttpEndpointData()
{
    HttpEndPointData_t data;

    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            (void)req;
            res.status = 200;
            res.set_content(statusJson().dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "status",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Get decision engine runtime status" });

    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            (void)req;
            std::scoped_lock lk(stateMutex);
            res.status = 200;
            res.set_content(lastDecisionResult.toJson().dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "lastDecisionResult",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Get the last completed decision turn" });

    data.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
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
        nlohmann::json({ { "type", "object" },
            { "properties", { { "text", { { "type", "string" } } } } },
            { "required", nlohmann::json::array({ "text" }) } }),
        "Run a decision turn from plain text input" });

    return data;
}

std::future<std::string> DecisionEngine::modifyStringUsingAIForEmotionalState(
    const std::string& input,
    const std::vector<Personality::EmotionSimple>& emotions,
    const std::shared_ptr<TheCubeServer::IRemoteConversationClient>& remoteConversationClient,
    const std::string& responseCategory,
    const nlohmann::json& additionalContext,
    const std::function<void(std::string)>& progressCB)
{
    if (!remoteConversationClient) {
        return std::async(std::launch::async, [input]() { return input; });
    }

    const auto profile = buildEmotionalStyleProfile(emotions);
    nlohmann::json context = {
        { "responseCategory", responseCategory.empty() ? "intent_result" : responseCategory },
        { "responseText", input },
        { "emotions", emotionsToJson(emotions) },
        { "styleProfile", {
            { "dominantEmotion", profile.dominantEmotion },
            { "dominantValue", profile.dominantValue },
            { "secondaryEmotion", profile.secondaryEmotion },
            { "secondaryValue", profile.secondaryValue },
            { "intensityBucket", profile.intensityBucket }
        } }
    };
    if (additionalContext.is_object()) {
        for (const auto& [key, value] : additionalContext.items()) {
            context[key] = value;
        }
    }
    return remoteConversationClient->getEmotionalRewriteAsync(input, context, progressCB);
}
