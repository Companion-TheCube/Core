// VoiceInteractionManager implementation

#include "voiceInteractionManager.h"

using namespace DecisionEngine;

VoiceInteractionManager::VoiceInteractionManager(const std::shared_ptr<SpeechIn>& speech,
    const std::shared_ptr<I_Transcriber>& trans,
    const std::shared_ptr<I_IntentRecognition>& intent,
    const std::shared_ptr<Personality::PersonalityManager>& personality)
    : speechIn(speech)
    , transcriber(trans)
    , intentRecognition(intent)
    , personalityManager(personality)
{
    SpeechIn::subscribeToWakeWordDetection([this]() { this->onWakeWordDetected(); });
}

VoiceInteractionManager::~VoiceInteractionManager()
{
    if (interactionThread.joinable())
        interactionThread.request_stop();
}

void VoiceInteractionManager::onWakeWordDetected()
{
    if (state != State::IDLE || interactionActive.load())
        return;

    interactionActive.store(true);
    state = State::LISTENING;
    interactionThread = std::jthread([this](std::stop_token st) { this->interactionLoop(st); });
}

void VoiceInteractionManager::interactionLoop(std::stop_token st)
{
    SpeechIn::pauseWakeWordStreaming();

    std::vector<int16_t> buffer;
    size_t silenceCount = 0;

    while (!st.stop_requested()) {
        auto opt = speechIn->audioQueue->pop();
        if (!opt)
            continue;
        auto& data = *opt;
        buffer.insert(buffer.end(), data.begin(), data.end());

        for (auto sample : data) {
            if (std::abs(sample) < 200)
                silenceCount++;
            else
                silenceCount = 0;
            if (silenceCount > SILENCE_TIMEOUT)
                st.stop();
        }
    }

    std::string text = transcriber->transcribeBuffer(reinterpret_cast<uint16_t*>(buffer.data()), buffer.size());

    intentRecognition->recognizeIntentAsync(text, [](std::shared_ptr<Intent> intent) {
        if (intent)
            intent->execute();
    });

    SpeechIn::resumeWakeWordStreaming();
    state = State::IDLE;
    interactionActive.store(false);
}
