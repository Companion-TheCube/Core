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


// Transcriber implementations: local (CubeWhisper) and remote (TheCubeServer).
// These are currently stubs. Buffer/stream methods should convert audio into
// text either by calling whisper.cpp locally or by streaming to the server.
#include "transcriber.h"
#include "audio/constants.h"
#include "transcriptionEvents.h"
#include <cmath>


namespace DecisionEngine {
// TODO:
// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper class. Whisper class should be initialized and a reference to it saved for future use.
LocalTranscriber::LocalTranscriber()
{
    if (!cubeWhisper)
        cubeWhisper = std::make_shared<CubeWhisper>();
}
LocalTranscriber::~LocalTranscriber()
{
}
std::string LocalTranscriber::transcribeBuffer(const int16_t* audio, size_t length)
{
    // Ensure CubeWhisper is initialized (lazy init for speed on cold start)
    if (!cubeWhisper)
        cubeWhisper = std::make_shared<CubeWhisper>();

    // Wrap the PCM buffer into a queue for CubeWhisper::transcribe
    auto q = std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>(8);
    // Reinterpret the incoming buffer as signed 16-bit PCM and push
    const int16_t* pcm16 = reinterpret_cast<const int16_t*>(audio);
    std::vector<int16_t> chunk(pcm16, pcm16 + length);
    q->push(std::move(chunk));
    // Signal end of stream with an empty vector
    q->push({});

    // Transcribe synchronously and return the resulting text
    auto fut = CubeWhisper::transcribe(q);
    return fut.get();
}
std::string LocalTranscriber::transcribeStream(const int16_t* audio, size_t bufSize)
{
    // TODO: consume from queue or stream interface; incremental transcription
    return "";
}

std::shared_ptr<ThreadSafeQueue<std::string>> LocalTranscriber::transcribeQueue(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
{
    auto textQueue = std::make_shared<ThreadSafeQueue<std::string>>(10);
    workerThread = std::jthread([this, audioQueue, textQueue](std::stop_token st) {
        // Streaming: maintain a rolling window and transcribe at a fixed step
        double minSec  = GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::TRANSCRIBER_MIN_SECONDS);
        double maxSec  = GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::TRANSCRIBER_MAX_SECONDS);
        double stepSec = GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::TRANSCRIBER_STEP_SECONDS);
        if (stepSec <= 0.05) stepSec = std::max(0.25, minSec/2.0);
        if (minSec <= 0.1) minSec = 0.5; // clamp to sane default
        if (maxSec < minSec) maxSec = std::max(minSec, 10.0);
        const size_t kMinSamples  = static_cast<size_t>(minSec  * audio::SAMPLE_RATE);
        const size_t kMaxSamples  = static_cast<size_t>(maxSec  * audio::SAMPLE_RATE);
        const size_t kStepSamples = static_cast<size_t>(stepSec * audio::SAMPLE_RATE);

        // VAD + hangover
        double vadTh   = GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::TRANSCRIBER_VAD_THRESHOLD); // normalized
        double hangSec = GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::TRANSCRIBER_VAD_HANGOVER_SECONDS);
        if (vadTh <= 0.0) vadTh = 0.015; // default
        if (hangSec < 0.1) hangSec = 0.25;

        std::vector<int16_t> window;
        window.reserve(kMaxSamples);
        size_t lastProcessed = 0; // number of samples processed so far
        std::string lastText;
        bool inSpeech = false;
        auto lastSpeech = std::chrono::steady_clock::now();
        auto lastStep   = std::chrono::steady_clock::now();

        while (!st.stop_requested()) {
            auto audioOpt = audioQueue->pop();
            if (!audioOpt) continue;
            const auto& blk = *audioOpt;
            if (blk.empty()) continue;
            // append and clip to max window
            if (window.size() + blk.size() > kMaxSamples) {
                size_t overflow = window.size() + blk.size() - kMaxSamples;
                if (overflow < window.size())
                    window.erase(window.begin(), window.begin() + overflow);
                else
                    window.clear();
                // reset processed baseline when we drop old data
                if (lastProcessed > window.size()) lastProcessed = 0;
            }
            window.insert(window.end(), blk.begin(), blk.end());

            // VAD on current block
            double sumsq = 0.0; for (int16_t s : blk) { double x = s/32768.0; sumsq += x*x; }
            double rms = blk.empty() ? 0.0 : std::sqrt(sumsq / blk.size());
            if (rms >= vadTh) { inSpeech = true; lastSpeech = std::chrono::steady_clock::now(); }

            auto now = std::chrono::steady_clock::now();
            if (inSpeech && window.size() >= kMinSamples &&
                ((window.size() - lastProcessed) >= kStepSamples || (now - lastStep) >= std::chrono::duration<double>(stepSec))) {
                CubeLog::info("LocalTranscriber: streaming transcribe win= " +
                               std::to_string(window.size() / static_cast<double>(audio::SAMPLE_RATE)) + "s");
                std::string text = CubeWhisper::transcribeSync(window);
                lastProcessed = window.size();
                lastStep = now;
                if (!text.empty() && text != lastText) {
                    lastText = text;
                    textQueue->push(text);
                    TranscriptionEvents::publish(text, false);
                }
            }

            // end of speech detection via hangover
            if (inSpeech && (now - lastSpeech) >= std::chrono::duration<double>(hangSec)) {
                inSpeech = false;
                CubeLog::info("LocalTranscriber: end-of-speech detected (hangover=" + std::to_string(hangSec) + "s)");
                if (!lastText.empty()) {
                    TranscriptionEvents::publish(lastText, true);
                }
                // reset state for next utterance
                window.clear();
                lastText.clear();
                lastProcessed = 0;
            }
        }
    });
    return textQueue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RemoteTranscriber - class that interacts with the TheCube Server API.
RemoteTranscriber::RemoteTranscriber()
{
    // TODO: inject server API, set up session/state for streaming
}
RemoteTranscriber::~RemoteTranscriber()
{
}
std::string RemoteTranscriber::transcribeBuffer(const int16_t* audio, size_t length)
{
    // TODO: upload buffer and poll for result
    return "";
}
std::string RemoteTranscriber::transcribeStream(const int16_t* audio, size_t bufSize)
{
    // TODO: streaming upload with progress and partial results
    return "";
}

std::shared_ptr<ThreadSafeQueue<std::string>> RemoteTranscriber::transcribeQueue(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
{
    auto textQueue = std::make_shared<ThreadSafeQueue<std::string>>(10);
    workerThread = std::jthread([this, audioQueue, textQueue](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            auto audioOpt = audioQueue->pop();
            if (audioOpt) {
                const auto& audioVec = *audioOpt;
                std::string transcription = transcribeBuffer(audioVec.data(), audioVec.size());
            }
        }
    });
    return textQueue;
}

}
