#include <gtest/gtest.h>

#include "decisionEngine/transcriber.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

class FakeRemoteAudioClient : public TheCubeServer::IRemoteAudioClient {
public:
    bool startStreamingTranscription() override
    {
        std::scoped_lock lock(mutex_);
        sessionStarted = true;
        finishCount = 0;
        cancelCount = 0;
        chunksSent = 0;
        return true;
    }

    bool sendAudioChunk(std::span<const int16_t> audio) override
    {
        std::scoped_lock lock(mutex_);
        if (audio.empty()) {
            return false;
        }
        ++chunksSent;
        return true;
    }

    bool finishStreamingTranscription() override
    {
        std::scoped_lock lock(mutex_);
        ++finishCount;
        return true;
    }

    std::string waitForFinalTranscript(std::chrono::milliseconds timeout) override
    {
        (void)timeout;
        std::scoped_lock lock(mutex_);
        return chunksSent >= 2 ? "what time is it" : std::string();
    }

    bool cancelStreamingTranscription() override
    {
        std::scoped_lock lock(mutex_);
        ++cancelCount;
        return true;
    }

    size_t chunkCount() const
    {
        std::scoped_lock lock(mutex_);
        return chunksSent;
    }

    int finishCalls() const
    {
        std::scoped_lock lock(mutex_);
        return finishCount;
    }

private:
    mutable std::mutex mutex_;
    bool sessionStarted = false;
    size_t chunksSent = 0;
    int finishCount = 0;
    int cancelCount = 0;
};

void resetTranscriberConfig()
{
    Config::erase("REMOTE_TRANSCRIPTION_SILENCE_TIMEOUT_MS");
    Config::erase("REMOTE_TRANSCRIPTION_WAIT_FINAL_MS");
    Config::erase("REMOTE_TRANSCRIPTION_NO_SPEECH_TIMEOUT_MS");
    Config::erase("REMOTE_TRANSCRIPTION_MAX_SESSION_MS");
    Config::erase("REMOTE_TRANSCRIPTION_WAKEWORD_VAD_DELAY_MS");
    Config::erase("SILERO_VAD_ENABLED");
}

bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

} // namespace

TEST(RemoteTranscriber, WakewordVadDelayPreventsWakeSpeechFromEndingTurnEarly)
{
    resetTranscriberConfig();
    Config::set("SILERO_VAD_ENABLED", "0");
    Config::set("REMOTE_TRANSCRIPTION_SILENCE_TIMEOUT_MS", "40");
    Config::set("REMOTE_TRANSCRIPTION_NO_SPEECH_TIMEOUT_MS", "250");
    Config::set("REMOTE_TRANSCRIPTION_MAX_SESSION_MS", "1000");
    Config::set("REMOTE_TRANSCRIPTION_WAIT_FINAL_MS", "50");
    Config::set("REMOTE_TRANSCRIPTION_WAKEWORD_VAD_DELAY_MS", "150");

    auto audioClient = std::make_shared<FakeRemoteAudioClient>();
    auto transcriber = std::make_unique<DecisionEngine::RemoteTranscriber>();
    transcriber->setRemoteClients(audioClient, nullptr);

    auto audioQueue = std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>(16);
    auto textQueue = transcriber->transcribeQueue(audioQueue);
    ASSERT_NE(textQueue, nullptr);

    audioQueue->push(std::vector<int16_t>(256, 2000));
    std::this_thread::sleep_for(std::chrono::milliseconds(170));
    audioQueue->push(std::vector<int16_t>(256, 2000));

    ASSERT_TRUE(waitUntil(
        [&textQueue]() { return textQueue->size() > 0; },
        std::chrono::milliseconds(1000)));

    auto result = textQueue->pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "what time is it");
    EXPECT_GE(audioClient->chunkCount(), 2u);
    EXPECT_EQ(audioClient->finishCalls(), 1);

    transcriber->interrupt();
    resetTranscriberConfig();
}
