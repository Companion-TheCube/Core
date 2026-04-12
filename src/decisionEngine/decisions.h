/*
██████╗ ███████╗ ██████╗██╗███████╗██╗ ██████╗ ███╗   ██╗███████╗   ██╗  ██╗
██╔══██╗██╔════╝██╔════╝██║██╔════╝██║██╔═══██╗████╗  ██║██╔════╝   ██║  ██║
██║  ██║█████╗  ██║     ██║███████╗██║██║   ██║██╔██╗ ██║███████╗   ███████║
██║  ██║██╔══╝  ██║     ██║╚════██║██║██║   ██║██║╚██╗██║╚════██║   ██╔══██║
██████╔╝███████╗╚██████╗██║███████║██║╚██████╔╝██║ ╚████║███████║██╗██║  ██║
╚═════╝ ╚══════╝ ╚═════╝╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License
Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once

#include "../api/autoRegister.h"
#include "../audio/audioManager.h"
#include "../database/chatHistoryStore.h"
#include "../threadsafeQueue.h"
#include "functionRegistry.h"
#include "intentRegistry.h"
#include "notificationCenter.h"
#include "personalityManager.h"
#include "remoteServer.h"
#include "scheduler.h"
#include "transcriber.h"
#include "transcriptionEvents.h"
#include "triggers.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

namespace DecisionEngine {

struct DecisionTurnResult {
    std::string transcript;
    std::string intentName;
    std::string capabilityName;
    std::string historyKey;
    std::string executionStatus;
    std::string responseText;
    nlohmann::json capabilityResult = nlohmann::json::object();
    std::string error;
    bool speakResult = false;
    int64_t timestampEpochMs = 0;

    nlohmann::json toJson() const
    {
        return nlohmann::json({ { "transcript", transcript },
            { "intentName", intentName },
            { "capabilityName", capabilityName },
            { "historyKey", historyKey },
            { "executionStatus", executionStatus },
            { "responseText", responseText },
            { "capabilityResult", capabilityResult },
            { "error", error },
            { "speakResult", speakResult },
            { "timestampEpochMs", timestampEpochMs } });
    }
};

class DecisionEngineMain : public AutoRegisterAPI<DecisionEngineMain> {
public:
    enum class TurnState {
        IDLE,
        WAKE_ACKNOWLEDGED,
        LISTENING,
        TRANSCRIPT_STREAMING,
        FINAL_TRANSCRIPT_PENDING_INTENT,
        INTENT_EXECUTING,
        RESULT_PRESENTED
    };

    DecisionEngineMain();
    ~DecisionEngineMain();

    void start();
    void stop();
    void restart();
    void pause();
    void resume();

    HttpEndPointData_t getHttpEndpointData() override;
    std::string getInterfaceName() const override { return "DecisionEngine"; }

private:
    struct TurnTimingMetrics {
        uint64_t turnId = 0;
        int64_t wakeDetectedEpochMs = 0;
        int64_t listeningUiShownEpochMs = 0;
        int64_t firstPartialEpochMs = 0;
        int64_t finalTranscriptEpochMs = 0;
        int64_t intentResolvedEpochMs = 0;
        int64_t resultPresentedEpochMs = 0;
        int64_t speechRequestedEpochMs = 0;
        std::string voiceSessionId;
        size_t partialEventCount = 0;
        size_t lastTranscriptLength = 0;
        bool summaryLogged = false;
    };

    DecisionTurnResult processTranscript(const std::string& transcript);
    DecisionTurnResult executeIntent(const std::shared_ptr<Intent>& intent, const std::string& transcript, const nlohmann::json& resolvedArgs = nlohmann::json::object());
    void recordTurnResult(const DecisionTurnResult& result);
    nlohmann::json statusJson() const;
    void stopTranscriptionConsumer();
    void onWakeWordDetected();
    void handleTranscriptEvent(const TranscriptionEvent& event);
    void showListeningUi();
    void presentTurnResult(const DecisionTurnResult& result);
    void hideTurnUi();
    DecisionTurnResult makeVoiceServiceUnavailableResult(const std::string& transcript) const;
    bool hasRemoteVoiceServiceFailure() const;
    void scheduleReturnToIdle(std::chrono::milliseconds delay);
    void setTurnState(TurnState newState);
    void beginTurnTiming();
    void noteListeningUiShown();
    void noteFirstPartial(const TranscriptionEvent& event);
    void noteFinalTranscript(const std::string& transcript);
    void noteIntentResolved(const TheCubeServer::ResolvedIntentCall& resolved);
    void noteResultPresented(const DecisionTurnResult& result, const std::string& displayMessage);
    void noteSpeechRequested();
    void finalizeTurnTiming(const std::string& reason, const DecisionTurnResult* result = nullptr);
    void maybeCaptureVoiceSessionIdLocked();
    void logTurnTimingStage(const TurnTimingMetrics& snapshot, const std::string& stage, int64_t stageEpochMs, const std::string& extra = "") const;

    std::shared_ptr<I_IntentRecognition> intentRecognition;
    std::shared_ptr<IntentRegistry> intentRegistry;
    std::shared_ptr<FunctionRegistry> functionRegistry;
    std::shared_ptr<NotificationCenter> notificationCenter;
    std::shared_ptr<I_Transcriber> transcriber;
    std::shared_ptr<Scheduler> scheduler;
    std::shared_ptr<TriggerManager> triggerManager;
    std::shared_ptr<Personality::PersonalityManager> personalityManager;
    std::shared_ptr<TheCubeServer::TheCubeServerAPI> remoteServerAPI;
    std::shared_ptr<ChatHistoryStore> chatHistoryStore;

    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue;
    std::shared_ptr<ThreadSafeQueue<std::string>> transcription;
    std::jthread transcriptionConsumerThread;

    mutable std::mutex stateMutex;
    DecisionTurnResult lastDecisionResult;
    TurnTimingMetrics currentTurnTiming;
    uint64_t nextTurnId = 1;
    std::string latestTranscriptEvent;
    TurnState turnState = TurnState::IDLE;
    std::string activeTranscriptPreview;
    bool started = false;
    std::atomic<uint64_t> resultPresentationGeneration { 0 };
    size_t wakeWordCallbackHandle = std::numeric_limits<size_t>::max();
    size_t wakeAudioQueueHandle = std::numeric_limits<size_t>::max();
    size_t transcriptionEventHandle = std::numeric_limits<size_t>::max();
};

std::vector<IntentCTorParams> getSystemIntents();
std::vector<IntentCTorParams> getSystemSchedule();

std::future<std::string> modifyStringUsingAIForEmotionalState(
    const std::string& input,
    const std::vector<Personality::EmotionSimple>& emotions,
    const std::shared_ptr<TheCubeServer::IRemoteConversationClient>& remoteConversationClient,
    const std::string& responseCategory = "",
    const nlohmann::json& additionalContext = nlohmann::json::object(),
    const std::function<void(std::string)>& progressCB = [](std::string) {});

} // namespace DecisionEngine
