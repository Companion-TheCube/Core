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


namespace DecisionEngine {
// TODO:
// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper class. Whisper class should be initialized and a reference to it saved for future use.
LocalTranscriber::LocalTranscriber()
{
    // TODO: construct CubeWhisper and prime any resources required for STT
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
    workerThread = std::jthread([this, audioQueue, textQueue](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            auto audioOpt = audioQueue->pop();
            if (audioOpt) {
                const auto& audioVec = *audioOpt;
                std::string transcription = transcribeBuffer(audioVec.data(), audioVec.size());
                textQueue->push(transcription);
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