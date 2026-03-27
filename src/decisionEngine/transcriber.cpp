/*
████████╗██████╗  █████╗ ███╗   ██╗███████╗ ██████╗██████╗ ██╗██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗ 
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔════╝██╔══██╗██║██╔══██╗██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██║     ██████╔╝██║██████╔╝█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██║     ██╔══██╗██║██╔══██╗██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║╚██████╗██║  ██║██║██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║     
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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


// Transcriber implementation: remote (TheCubeServer).
// Buffer/stream methods should convert audio into text by streaming to the server.
#include "transcriber.h"
#include "transcriptionEvents.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>
#include <type_traits>

namespace {

template <typename T>
T parseNumericConfig(const std::string& key, T fallback)
{
    try {
        const std::string raw = Config::get(key, "");
        if (raw.empty()) return fallback;
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(std::stod(raw));
        } else {
            return static_cast<T>(std::stoll(raw));
        }
    } catch (...) {
        return fallback;
    }
}

std::string formatDurationMs(const std::chrono::steady_clock::time_point& start, const std::chrono::steady_clock::time_point& end = std::chrono::steady_clock::now())
{
    const auto durationMs = std::chrono::duration<double, std::milli>(end - start).count();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << durationMs;
    return oss.str();
}

} // namespace

namespace DecisionEngine {
RemoteTranscriber::RemoteTranscriber()
{
    sessionActive = false;
}
RemoteTranscriber::~RemoteTranscriber()
{
    interrupt();
    cancelActiveSession();
}
std::string RemoteTranscriber::transcribeBuffer(const int16_t* audio, size_t length)
{
    if (!audio || length == 0 || !remoteAudioClient) return std::string();
    if (!initTranscribing()) return std::string();

    if (!remoteAudioClient->sendAudioChunk(std::span<const int16_t>(audio, length))) {
        CubeLog::error("RemoteTranscriber: failed to stream buffer payload");
        cancelActiveSession();
        return std::string();
    }

    if (!remoteAudioClient->finishStreamingTranscription()) {
        CubeLog::error("RemoteTranscriber: failed to finish transcription session");
        cancelActiveSession();
        return std::string();
    }

    const auto finalWaitMs = parseNumericConfig<int>("REMOTE_TRANSCRIPTION_WAIT_FINAL_MS", 15000);
    std::string transcript = remoteAudioClient->waitForFinalTranscript(std::chrono::milliseconds(finalWaitMs));
    if (!transcript.empty()) {
        TranscriptionEvents::publish({ .fullText = transcript, .appendText = "", .isFinal = true });
    }
    cancelActiveSession();
    return transcript;
}
std::string RemoteTranscriber::transcribeStream(const int16_t* audio, size_t bufSize)
{
    return transcribeBuffer(audio, bufSize);
}

std::shared_ptr<ThreadSafeQueue<std::string>> RemoteTranscriber::transcribeQueue(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
{
    auto textQueue = std::make_shared<ThreadSafeQueue<std::string>>(10);
    if (!remoteAudioClient) {
        CubeLog::error("RemoteTranscriber: remote audio client is not set");
        return nullptr;
    }
    if (!audioQueue) {
        CubeLog::error("RemoteTranscriber: audio queue is null");
        return nullptr;
    }
    this->audioQueue = audioQueue;

    if (workerThread.joinable()) {
        workerThread.request_stop();
        cancelActiveSession();
    }
    const auto startupStart = std::chrono::steady_clock::now();
    CubeLog::info("RemoteTranscriber: starting utterance session");
    drainAudioQueue(audioQueue);

    if (!initTranscribing()) {
        CubeLog::error(
            "RemoteTranscriber: failed to create remote transcription session"
            " startupMs=" + formatDurationMs(startupStart));
        return nullptr;
    }
    sessionActive = true;
    const auto startupReadyAt = std::chrono::steady_clock::now();
    CubeLog::info(
        "RemoteTranscriber: remote transcription session ready"
        " startupMs=" + formatDurationMs(startupStart, startupReadyAt));

    workerThread = std::jthread([this, audioQueue, textQueue, startupMs = formatDurationMs(startupStart, startupReadyAt)](std::stop_token stoken) {
        const auto signalFailure = [&textQueue]() {
            if (textQueue) {
                textQueue->push(std::string());
            }
        };
        const auto maybeElapsedMs = [](const std::optional<std::chrono::steady_clock::time_point>& maybeStart, const std::chrono::steady_clock::time_point& sessionStart) {
            if (!maybeStart.has_value()) {
                return std::string("n/a");
            }
            return formatDurationMs(sessionStart, *maybeStart);
        };
        const auto silenceTimeout = std::chrono::milliseconds(parseNumericConfig<int>("REMOTE_TRANSCRIPTION_SILENCE_TIMEOUT_MS", 450));
        const auto noSpeechTimeout = std::chrono::milliseconds(parseNumericConfig<int>("REMOTE_TRANSCRIPTION_NO_SPEECH_TIMEOUT_MS", 5000));
        const auto maxSessionDuration = std::chrono::milliseconds(parseNumericConfig<int>("REMOTE_TRANSCRIPTION_MAX_SESSION_MS", 15000));
        const auto finalWait = std::chrono::milliseconds(parseNumericConfig<int>("REMOTE_TRANSCRIPTION_WAIT_FINAL_MS", 15000));
        const auto idlePoll = std::chrono::milliseconds(10);

        size_t chunksSent = 0;
        bool heardSpeech = false;
        auto sessionStart = std::chrono::steady_clock::now();
        auto lastSpeechAt = sessionStart;
        std::optional<std::chrono::steady_clock::time_point> firstChunkAt;
        std::optional<std::chrono::steady_clock::time_point> firstSpeechAt;
        std::optional<std::chrono::steady_clock::time_point> finishRequestedAt;
        std::optional<std::chrono::steady_clock::time_point> finalTranscriptAt;
        std::string endReason = "unknown";
        const auto logSessionSummary = [&](const std::string& reason, size_t transcriptLength) {
            CubeLog::info(
                "RemoteTranscriber: utterance session summary"
                " reason=" + reason
                + ", startupMs=" + startupMs
                + ", sessionMs=" + formatDurationMs(sessionStart)
                + ", firstChunkMs=" + maybeElapsedMs(firstChunkAt, sessionStart)
                + ", firstSpeechMs=" + maybeElapsedMs(firstSpeechAt, sessionStart)
                + ", finishRequestedMs=" + maybeElapsedMs(finishRequestedAt, sessionStart)
                + ", finalTranscriptMs=" + maybeElapsedMs(finalTranscriptAt, sessionStart)
                + ", chunksSent=" + std::to_string(chunksSent)
                + ", transcriptLength=" + std::to_string(transcriptLength));
        };

        while (!stoken.stop_requested()) {
            const auto now = std::chrono::steady_clock::now();
            if ((now - sessionStart) >= maxSessionDuration) {
                CubeLog::warning("RemoteTranscriber: max session duration reached, finishing session");
                endReason = "max_session_duration";
                break;
            }

            if (audioQueue->size() == 0) {
                if (!heardSpeech && (now - sessionStart) >= noSpeechTimeout) {
                    CubeLog::warning("RemoteTranscriber: no speech detected before timeout, finishing session");
                    endReason = "no_speech_timeout";
                    break;
                }
                if (heardSpeech && (now - lastSpeechAt) >= silenceTimeout) {
                    CubeLog::info("RemoteTranscriber: silence timeout reached, finishing session");
                    endReason = "silence_timeout";
                    break;
                }
                std::this_thread::sleep_for(idlePoll);
                continue;
            }

            auto audioOpt = audioQueue->pop();
            if (!audioOpt || audioOpt->empty()) continue;

            const auto chunkTime = std::chrono::steady_clock::now();
            if (!firstChunkAt.has_value()) {
                firstChunkAt = chunkTime;
                CubeLog::info(
                    "RemoteTranscriber: first audio chunk streamed"
                    " firstChunkMs=" + formatDurationMs(sessionStart, chunkTime)
                    + ", samples=" + std::to_string(audioOpt->size()));
            }
            if (!remoteAudioClient->sendAudioChunk(std::span<const int16_t>(audioOpt->data(), audioOpt->size()))) {
                CubeLog::error("RemoteTranscriber: failed while streaming audio; cancelling session");
                cancelActiveSession();
                sessionActive = false;
                endReason = "stream_send_failed";
                logSessionSummary(endReason, 0);
                signalFailure();
                return;
            }

            ++chunksSent;
            if (isSpeechChunk(*audioOpt)) {
                heardSpeech = true;
                lastSpeechAt = chunkTime;
                if (!firstSpeechAt.has_value()) {
                    firstSpeechAt = chunkTime;
                    CubeLog::info(
                        "RemoteTranscriber: first speech chunk detected"
                        " firstSpeechMs=" + formatDurationMs(sessionStart, chunkTime)
                        + ", chunksSent=" + std::to_string(chunksSent));
                }
            } else if (heardSpeech && (chunkTime - lastSpeechAt) >= silenceTimeout) {
                CubeLog::info("RemoteTranscriber: post-speech silence timeout reached, finishing session");
                endReason = "post_speech_silence_timeout";
                break;
            } else if (!heardSpeech && (chunkTime - sessionStart) >= noSpeechTimeout) {
                CubeLog::warning("RemoteTranscriber: no speech detected in session window, finishing session");
                endReason = "no_speech_window";
                break;
            }
        }

        if (stoken.stop_requested()) {
            CubeLog::info("RemoteTranscriber: interrupted; cancelling active session");
            cancelActiveSession();
            sessionActive = false;
            drainAudioQueue(audioQueue);
            logSessionSummary("interrupted", 0);
            signalFailure();
            return;
        }

        if (!sessionActive) {
            CubeLog::warning("RemoteTranscriber: no active session found at finalize step");
            sessionActive = false;
            logSessionSummary("no_active_session", 0);
            signalFailure();
            return;
        }

        if (chunksSent == 0) {
            CubeLog::warning("RemoteTranscriber: no audio chunks sent; cancelling session");
            cancelActiveSession();
            sessionActive = false;
            logSessionSummary("no_audio_chunks", 0);
            signalFailure();
            return;
        }

        finishRequestedAt = std::chrono::steady_clock::now();
        CubeLog::info(
            "RemoteTranscriber: finish requested"
            " chunks=" + std::to_string(chunksSent)
            + ", finishRequestedMs=" + formatDurationMs(sessionStart, *finishRequestedAt)
            + ", endReason=" + endReason);
        if (!remoteAudioClient->finishStreamingTranscription()) {
            CubeLog::error("RemoteTranscriber: finish failed; cancelling session");
            cancelActiveSession();
            sessionActive = false;
            logSessionSummary("finish_failed", 0);
            signalFailure();
            return;
        }

        std::string transcription = remoteAudioClient->waitForFinalTranscript(finalWait);
        sessionActive = false;

        if (!transcription.empty()) {
            finalTranscriptAt = std::chrono::steady_clock::now();
            CubeLog::info(
                "RemoteTranscriber: final transcript received"
                " transcriptLength=" + std::to_string(transcription.size())
                + ", finalTranscriptMs=" + formatDurationMs(sessionStart, *finalTranscriptAt));
            TranscriptionEvents::publish({ .fullText = transcription, .appendText = "", .isFinal = true });
            textQueue->push(transcription);
            logSessionSummary("success", transcription.size());
        } else {
            CubeLog::warning("RemoteTranscriber: final transcript missing or empty");
            logSessionSummary("final_transcript_missing", 0);
            signalFailure();
        }

        drainAudioQueue(audioQueue);
    });
    CubeLog::info("RemoteTranscriber: transcription worker started");
    return textQueue;
}

bool RemoteTranscriber::initTranscribing()
{
    if (!remoteAudioClient) {
        CubeLog::error("RemoteTranscriber: initTranscribing called without remote audio client");
        return false;
    }
    const bool created = remoteAudioClient->startStreamingTranscription();
    if (created && remoteServerAPI) {
        if (auto session = remoteServerAPI->getActiveTranscriptionSession(); session.has_value()) {
            CubeLog::info("RemoteTranscriber: remote session created id=" + session->sessionId);
        }
    }
    return created;
}

bool RemoteTranscriber::streamAudio()
{
    if (!remoteAudioClient || !audioQueue) return false;
    if (audioQueue->size() == 0) return true;
    auto audioOpt = audioQueue->pop();
    if (!audioOpt || audioOpt->empty()) return true;
    return remoteAudioClient->sendAudioChunk(std::span<const int16_t>(audioOpt->data(), audioOpt->size()));
}

bool RemoteTranscriber::stopTranscribing()
{
    if (!remoteAudioClient) return false;
    if (!remoteAudioClient->finishStreamingTranscription()) {
        return false;
    }
    std::string finalText = remoteAudioClient->waitForFinalTranscript(std::chrono::milliseconds(15000));
    if (!finalText.empty()) {
        TranscriptionEvents::publish({ .fullText = finalText, .appendText = "", .isFinal = true });
    }
    return !finalText.empty();
}

bool RemoteTranscriber::isSpeechChunk(const std::vector<int16_t>& audioChunk) const
{
    if (audioChunk.empty()) return false;

    const double meanAbsThreshold = parseNumericConfig<double>("REMOTE_TRANSCRIPTION_SPEECH_MEAN_ABS_THRESHOLD", 350.0);
    const int peakThreshold = parseNumericConfig<int>("REMOTE_TRANSCRIPTION_SPEECH_PEAK_THRESHOLD", 1200);

    long long sumAbs = 0;
    int peak = 0;
    for (int16_t sample : audioChunk) {
        int value = std::abs(static_cast<int>(sample));
        sumAbs += value;
        peak = std::max(peak, value);
    }

    const double meanAbs = static_cast<double>(sumAbs) / static_cast<double>(audioChunk.size());
    return meanAbs >= meanAbsThreshold || peak >= peakThreshold;
}

void RemoteTranscriber::drainAudioQueue(const std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>& queue) const
{
    if (!queue) return;
    size_t drained = 0;
    while (queue->size() > 0) {
        auto _ = queue->pop();
        (void)_;
        ++drained;
    }
    if (drained > 0) {
        CubeLog::debug("RemoteTranscriber: drained stale audio chunks count=" + std::to_string(drained));
    }
}

void RemoteTranscriber::cancelActiveSession() const
{
    if (!remoteAudioClient) return;
    if (!remoteAudioClient->cancelStreamingTranscription()) {
        CubeLog::warning("RemoteTranscriber: cancelStreamingTranscription failed");
    }
}

}
