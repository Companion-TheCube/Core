// VoiceInteractionManager
// This class coordinates audio flow for voice interactions.
// TODO: Implement according to plan in comments.

#pragma once
#include "speechIn.h"
#include "../decisionEngine/decisions.h"
#include "../decisionEngine/personalityManager.h"
#include <memory>
#include <atomic>
#include <vector>

/**
 * @brief VoiceInteractionManager handles the life cycle of a voice interaction.
 *
 * Responsibilities:
 *  - Receive wake word events from SpeechIn.
 *  - On wake word, optionally send the preTriggerBuffer to the server for missed wake word detection.
 *  - Switch audio routing from openwakeword to the configured STT provider.
 *  - Stream microphone audio to the server or local STT engine until silence is detected.
 *  - Provide hooks for sending the PersonalityManager emotional state with the first audio packet.
 *  - Notify the DecisionEngine when transcription and interpretation are complete.
 *  - Handle optional TTS playback once a response is available.
 *
 * Implementation notes:
 *  - This manager should be created by the DecisionEngineMain and hold references
 *    to SpeechIn, TheCubeServerAPI (or local STT), PersonalityManager and AudioOutput.
 *  - Streaming destinations must be configurable (remote server or local whisper.cpp).
 *  - Silence detection will reuse code from SpeechIn or implement a new routine.
 */

class VoiceInteractionManager {
public:
    VoiceInteractionManager(const std::shared_ptr<SpeechIn>& speech,
        const std::shared_ptr<DecisionEngine::I_Transcriber>& trans,
        const std::shared_ptr<DecisionEngine::I_IntentRecognition>& intent,
        const std::shared_ptr<Personality::PersonalityManager>& personality);
    ~VoiceInteractionManager();

    /** Begin a voice session after a wake word. */
    void onWakeWordDetected();

private:
    enum class State { IDLE, LISTENING } state = State::IDLE;

    std::shared_ptr<SpeechIn> speechIn;
    std::shared_ptr<DecisionEngine::I_Transcriber> transcriber;
    std::shared_ptr<DecisionEngine::I_IntentRecognition> intentRecognition;
    std::shared_ptr<Personality::PersonalityManager> personalityManager;

    std::jthread interactionThread;
    std::atomic<bool> interactionActive = false;

    void interactionLoop(std::stop_token st);
};

#endif // VOICEINTERACTIONMANAGER_H
