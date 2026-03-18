/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#pragma once

#include "../api/autoRegister.h"
#include "../audio/audioManager.h"
#include "../threadsafeQueue.h"
#include "functionRegistry.h"
#include "intentRegistry.h"
#include "personalityManager.h"
#include "remoteServer.h"
#include "scheduler.h"
#include "transcriber.h"
#include "triggers.h"
#include "transcriptionEvents.h"

#include <atomic>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

namespace DecisionEngine {

struct DecisionTurnResult {
    std::string transcript;
    std::string intentName;
    std::string executionStatus;
    std::string responseText;
    nlohmann::json capabilityResult = nlohmann::json::object();
    std::string error;
    int64_t timestampEpochMs = 0;

    nlohmann::json toJson() const
    {
        return nlohmann::json({
            { "transcript", transcript },
            { "intentName", intentName },
            { "executionStatus", executionStatus },
            { "responseText", responseText },
            { "capabilityResult", capabilityResult },
            { "error", error },
            { "timestampEpochMs", timestampEpochMs }
        });
    }
};

class DecisionEngineMain : public AutoRegisterAPI<DecisionEngineMain> {
public:
    DecisionEngineMain();
    ~DecisionEngineMain();

    void start();
    void stop();
    void restart();
    void pause();
    void resume();

    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override { return "DecisionEngine"; }

private:
    DecisionTurnResult processTranscript(const std::string& transcript);
    DecisionTurnResult executeIntent(const std::shared_ptr<Intent>& intent, const std::string& transcript);
    void recordTurnResult(const DecisionTurnResult& result);
    nlohmann::json statusJson() const;
    nlohmann::json parametersToJson(const Parameters& parameters) const;
    void applyCapabilityResultToIntent(const std::shared_ptr<Intent>& intent, const nlohmann::json& capabilityResult);
    void stopTranscriptionConsumer();
    void onWakeWordDetected();

    std::shared_ptr<I_IntentRecognition> intentRecognition;
    std::shared_ptr<IntentRegistry> intentRegistry;
    std::shared_ptr<FunctionRegistry> functionRegistry;
    std::shared_ptr<I_Transcriber> transcriber;
    std::shared_ptr<Scheduler> scheduler;
    std::shared_ptr<TriggerManager> triggerManager;
    std::shared_ptr<Personality::PersonalityManager> personalityManager;
    std::shared_ptr<TheCubeServer::TheCubeServerAPI> remoteServerAPI;

    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue;
    std::shared_ptr<ThreadSafeQueue<std::string>> transcription;
    std::jthread transcriptionConsumerThread;

    mutable std::mutex stateMutex;
    DecisionTurnResult lastDecisionResult;
    std::string latestTranscriptEvent;
    bool started = false;
    size_t wakeWordCallbackHandle = std::numeric_limits<size_t>::max();
    size_t wakeAudioQueueHandle = std::numeric_limits<size_t>::max();
    size_t transcriptionEventHandle = std::numeric_limits<size_t>::max();
};

std::vector<IntentCTorParams> getSystemIntents();
std::vector<IntentCTorParams> getSystemSchedule();

std::future<std::string> modifyStringUsingAIForEmotionalState(
    const std::string& input,
    const std::vector<Personality::EmotionSimple>& emotions,
    const std::shared_ptr<TheCubeServer::TheCubeServerAPI>& remoteServerAPI,
    const std::function<void(std::string)>& progressCB);

} // namespace DecisionEngine
