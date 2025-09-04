/*
 ██████╗██╗   ██╗██████╗ ███████╗██╗    ██╗██╗  ██╗██╗███████╗██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗
██╔════╝██║   ██║██╔══██╗██╔════╝██║    ██║██║  ██║██║██╔════╝██╔══██╗██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██║     ██║   ██║██████╔╝█████╗  ██║ █╗ ██║███████║██║███████╗██████╔╝█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██║     ██║   ██║██╔══██╗██╔══╝  ██║███╗██║██╔══██║██║╚════██║██╔═══╝ ██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
╚██████╗╚██████╔╝██████╔╝███████╗╚███╔███╔╝██║  ██║██║███████║██║     ███████╗██║  ██║██╗╚██████╗██║     ██║
 ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

// CubeWhisper implementation: placeholder that will wrap whisper.cpp APIs.
// For now, logs initialization and returns a canned transcription.
#include "cubeWhisper.h"
#include <future>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <array>
#include <string_view>
#include <thread>
#include <chrono>

// static members
whisper_context* CubeWhisper::ctx = nullptr;
std::mutex CubeWhisper::partialMutex;
std::string CubeWhisper::partialResult;
std::jthread CubeWhisper::transcriberThread;

namespace {
bool hasWhisperMagic(const std::filesystem::path& p)
{
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs) return false;
    char hdr[4] = {0};
    ifs.read(hdr, 4);
    return (ifs.gcount() == 4) &&
           ((hdr[0]=='l'&&hdr[1]=='m'&&hdr[2]=='g'&&hdr[3]=='g') ||
            (hdr[0]=='G'&&hdr[1]=='G'&&hdr[2]=='U'&&hdr[3]=='F'));
}

std::string resolveWhisperModelPath()
{
    // Priority 1: environment variable
    if (const char* env = std::getenv("WHISPER_MODEL_PATH")) {
        if (std::filesystem::exists(env)) return std::string(env);
        CubeLog::warning(std::string("WHISPER_MODEL_PATH set but not found: ") + env);
    }
    // Priority 2: alongside binary: <bin>/whisper_models/*.{bin,gguf}
    char exePath[4096] = {0};
    ssize_t len = ::readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0) {
        std::filesystem::path p(exePath);
        auto base = p.parent_path();
        auto dir = base / "whisper_models";
        if (std::filesystem::exists(dir)) {
            static constexpr std::array<std::string_view, 5> prefs = {
                "ggml-tiny.bin",
                "ggml-large-v3.bin",
                "ggml-large.bin",
                "ggml-medium.en.bin",
                "ggml-base.en.bin"
            };
            for (std::string_view name : prefs) {
                auto cand = dir / std::string(name);
                CubeLog::info("Checking for whisper model at " + cand.string());
                if (std::filesystem::exists(cand) && hasWhisperMagic(cand)) return cand.string();
            }
            for (auto& entry : std::filesystem::directory_iterator(dir)) {
                auto sp = entry.path();
                if (!sp.has_extension()) continue;
                auto ext = sp.extension().string();
                if (ext == ".bin" || ext == ".gguf") {
                    if (hasWhisperMagic(sp)) return sp.string();
                }
            }
        }
    }
    // Priority 3: CWD-relative: scan directory
    if (std::filesystem::exists("whisper_models")) {
        for (auto& entry : std::filesystem::directory_iterator("whisper_models")) {
            auto sp = entry.path();
            if (!sp.has_extension()) continue;
            auto ext = sp.extension().string();
            if (ext == ".bin" || ext == ".gguf") {
                if (hasWhisperMagic(sp)) return sp.string();
            }
        }
    }
    // Priority 4: repo path
    if (std::filesystem::exists("libraries/whisper_models")) {
        for (auto& entry : std::filesystem::directory_iterator("libraries/whisper_models")) {
            auto sp = entry.path();
            if (!sp.has_extension()) continue;
            auto ext = sp.extension().string();
            if (ext == ".bin" || ext == ".gguf") {
                if (hasWhisperMagic(sp)) return sp.string();
            }
        }
    }
    return {};
}
}

CubeWhisper::CubeWhisper()
{
    if (!ctx) {
        auto params = whisper_context_params{
            .use_gpu = false,
            .flash_attn = false,
            .gpu_device = -1,
            .dtw_token_timestamps = false,
            .dtw_aheads_preset = WHISPER_AHEADS_NONE,
            .dtw_n_top = 1,
            .dtw_aheads = {0},
            .dtw_mem_size = 0,
        };
        auto modelPath = resolveWhisperModelPath();
        if (modelPath.empty()) {
            CubeLog::error("CubeWhisper: Whisper model not found in expected locations");
        } else {
            CubeLog::info("CubeWhisper: loading model from " + modelPath);
            ctx = whisper_init_from_file_with_params(modelPath.c_str(), params);
        }
        if (!ctx)
            CubeLog::error("CubeWhisper: failed to load Whisper model (bad magic or incompatible format). "
                            "Ensure you have a ggml/gguf model like ggml-large-v3.bin in whisper_models.");
        else
            CubeLog::info("CubeWhisper: model loaded successfully");
    }
}

/*
 * Transcribe audio from the provided ThreadSafeQueue.
 * The queue should provide vectors of int16_t PCM samples (16kHz mono).
 * Returns a future that will hold the final transcription string.
 * Partial transcriptions can be retrieved via getPartialTranscription().
 * Place an empty vector in the queue to signal end of input.
 */
std::future<std::string> CubeWhisper::transcribe(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
{
    CubeLog::info("");
    std::promise<std::string> promise;
    auto future = promise.get_future();

    if (!ctx) {
        CubeLog::error("Whisper context not initialized");
        promise.set_value("");
        return future;
    }

    transcriberThread = std::jthread(
        [audioQueue, p = std::move(promise)](std::stop_token st) mutable {
            CubeLog::info("");
            whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
            params.print_progress = true;
            params.print_realtime = false;
            params.print_timestamps = false;

            std::vector<float> pcmf32;
            {
                std::lock_guard<std::mutex> lk(partialMutex);
                partialResult.clear();
            }

            while (true) {
                auto dataOpt = audioQueue->pop();
                if (!dataOpt || dataOpt->empty()) {
                    CubeLog::info("");
                    break;
                }

                pcmf32.reserve(pcmf32.size() + dataOpt->size());
                for (int16_t s : *dataOpt)
                    pcmf32.push_back(static_cast<float>(s) / 32768.0f);
                CubeLog::info("");
                if (whisper_full(ctx, params, pcmf32.data(), pcmf32.size()) != 0) {
                    CubeLog::error("whisper_full failed");
                    break;
                }
                CubeLog::info("");
                std::stringstream ss;
                int n = whisper_full_n_segments(ctx);
                for (int i = 0; i < n; ++i)
                    ss << whisper_full_get_segment_text(ctx, i);

                {
                    std::lock_guard<std::mutex> lk(partialMutex);
                    partialResult = ss.str();
                }

                if (st.stop_requested()) {
                    break;
                }
            }

            std::string result;
            {
                std::lock_guard<std::mutex> lk(partialMutex);
                result = partialResult;
            }
            p.set_value(result);
        });
    return future;
}

std::string CubeWhisper::getPartialTranscription()
{
    std::lock_guard<std::mutex> lk(partialMutex);
    return partialResult;
}

std::string CubeWhisper::transcribeSync(const std::vector<int16_t>& pcm16)
{
    // Ensure the Whisper context (model) has been loaded.
    if (!ctx) {
        CubeLog::error("CubeWhisper::transcribeSync: context not initialized");
        return {};
    }

    // Build decoding parameters similar to whisper.cpp examples (testing/stream.cpp),
    // but tailored for a single synchronous window (no rolling context).
    whisper_full_params decodeParams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    decodeParams.print_progress   = false;   // suppress progress prints
    decodeParams.print_realtime   = false;   // no realtime printing
    decodeParams.print_timestamps = false;   // plain text output
    decodeParams.no_context       = true;    // independent call, no prior context
    decodeParams.single_segment   = true;    // prefer a single segment result
    decodeParams.translate        = false;   // transcribe, do not translate
    decodeParams.max_len          = 0;       // no token limit
    decodeParams.language         = "en";    // assume English
    decodeParams.detect_language  = false;   // skip language detection
    unsigned numThreads           = std::max(1u, std::thread::hardware_concurrency());
    decodeParams.n_threads        = static_cast<int>(numThreads);

    // Convert PCM16 mono to float32 in [-1, 1] as required by Whisper.
    std::vector<float> audioF32;
    audioF32.reserve(pcm16.size());
    for (int16_t s : pcm16) audioF32.push_back(static_cast<float>(s) / 32768.0f);

    // Pre-call diagnostic to help trace long-running decodes.
    CubeLog::info(
        "CubeWhisper::transcribeSync: invoking whisper_full | samples=" + std::to_string(audioF32.size()) +
        ", threads=" + std::to_string(decodeParams.n_threads) +
        ", single_segment=true, no_context=true, lang='" + std::string(decodeParams.language ? decodeParams.language : "") + "'"
    );

    // Execute the decode. This is the heavyweight step and will block
    // until the model finishes processing the provided samples.
    auto tStart = std::chrono::steady_clock::now();
    if (whisper_full(ctx, decodeParams, audioF32.data(), audioF32.size()) != 0) {
        CubeLog::error("CubeWhisper::transcribeSync: whisper_full failed");
        return {};
    }
    auto tEnd = std::chrono::steady_clock::now();

    // Collect all decoded segments into a single string.
    std::stringstream textStream;
    int numSegments = whisper_full_n_segments(ctx);
    for (int i = 0; i < numSegments; ++i) textStream << whisper_full_get_segment_text(ctx, i);
    std::string fullText = textStream.str();

    // Normalize whitespace-only strings to empty to avoid spurious results.
    bool hasNonSpace = false; for (char c : fullText) { if (!std::isspace(static_cast<unsigned char>(c))) { hasNonSpace = true; break; } }
    if (!hasNonSpace) fullText.clear();

    // Telemetry log for timing and a short preview of the output.
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();
    std::string preview = fullText.substr(0, 160);
    CubeLog::info("CubeWhisper::transcribeSync: decode took " + std::to_string(elapsedMs) + "ms, segments=" + std::to_string(numSegments) +
                  ", samples=" + std::to_string(audioF32.size()) + ", text='" + preview + (fullText.size()>160?"…":"") + "'");
    return fullText;
}
