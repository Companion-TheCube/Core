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

// static members
whisper_context* CubeWhisper::ctx = nullptr;
std::mutex CubeWhisper::partialMutex;
std::string CubeWhisper::partialResult;
std::jthread CubeWhisper::transcriberThread;

CubeWhisper::CubeWhisper()
{
    if (!ctx) {
        ctx = whisper_init_from_file("libraries/whisper_models/large.bin");
        if (!ctx)
            CubeLog::error("Failed to load Whisper model");
    }
    CubeLog::info("CubeWhisper initialized");
}

std::future<std::string> CubeWhisper::transcribe(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
{
    std::promise<std::string> promise;
    auto future = promise.get_future();

    if (!ctx) {
        CubeLog::error("Whisper context not initialized");
        promise.set_value("");
        return future;
    }

    transcriberThread = std::jthread(
        [audioQueue, p = std::move(promise)]() mutable {
            whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
            params.print_progress = false;
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
                    break;
                }

                pcmf32.reserve(pcmf32.size() + dataOpt->size());
                for (int16_t s : *dataOpt)
                    pcmf32.push_back(static_cast<float>(s) / 32768.0f);

                if (whisper_full(ctx, params, pcmf32.data(), pcmf32.size()) != 0) {
                    CubeLog::error("whisper_full failed");
                    break;
                }

                std::stringstream ss;
                int n = whisper_full_n_segments(ctx);
                for (int i = 0; i < n; ++i)
                    ss << whisper_full_get_segment_text(ctx, i);

                {
                    std::lock_guard<std::mutex> lk(partialMutex);
                    partialResult = ss.str();
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
