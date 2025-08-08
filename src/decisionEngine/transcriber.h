/*
████████╗██████╗  █████╗ ███╗   ██╗███████╗ ██████╗██████╗ ██╗██████╗ ███████╗██████╗    ██╗  ██╗
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔════╝██╔══██╗██║██╔══██╗██╔════╝██╔══██╗   ██║  ██║
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██║     ██████╔╝██║██████╔╝█████╗  ██████╔╝   ███████║
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██║     ██╔══██╗██║██╔══██╗██╔══╝  ██╔══██╗   ██╔══██║
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║╚██████╗██║  ██║██║██████╔╝███████╗██║  ██║██╗██║  ██║
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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


#pragma once
#include "../database/cubeDB.h"
#include "utils.h"
#ifndef LOGGER_H
#include <logger.h>
#endif
#ifndef API_I_H
#include "../api/api.h"
#endif
#include "cubeWhisper.h"
#include "nlohmann/json.hpp"
#include "remoteServer.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../api/autoRegister.h"
#include "../audio/audioManager.h"
#include "../threadsafeQueue.h"
#include "globalSettings.h"
#include "httplib.h"
#include "personalityManager.h"
#include "intentRegistry.h"
#include "functionRegistry.h"
#include "remoteApi.h"

// Transcriber interfaces: convert audio streams/buffers into text
//
// Layers
// - I_AudioQueue: helper base providing a shared audio queue handle
// - I_Transcriber: abstract interface for buffer/stream transcription
// - LocalTranscriber: uses CubeWhisper (whisper.cpp) on-device
// - RemoteTranscriber: streams audio to TheCubeServer for transcription
//
// Threading
// - Implementations may spin worker threads or use async; interface itself is not thread-hostile
// - Audio queue ownership is external and injected via setThreadSafeQueue()
namespace DecisionEngine{



class I_AudioQueue {
public:
    virtual ~I_AudioQueue() = default;
    void setThreadSafeQueue(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
    {
        this->audioQueue = audioQueue;
    }
    const std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> getThreadSafeQueue()
    {
        return audioQueue;
    }

protected:
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue;
};


// Abstract transcriber. Implement buffer-based and streaming APIs.
class I_Transcriber: public I_AudioQueue {
public:
    virtual ~I_Transcriber() = default;
    virtual std::string transcribeBuffer(const uint16_t* audio, size_t length) = 0;
    virtual std::string transcribeStream(const uint16_t* audio, size_t bufSize) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

// On-device STT using CubeWhisper
class LocalTranscriber : public I_Transcriber {
public:
    LocalTranscriber();
    ~LocalTranscriber();
    std::string transcribeBuffer(const uint16_t* audio, size_t length) override;
    std::string transcribeStream(const uint16_t* audio, size_t bufSize) override;
    // TODO: the stream that this is reading from may need to be a more complex
    // datatype that has read and write pointers and a mutex to protect them.

private:
    std::shared_ptr<CubeWhisper> cubeWhisper;
};

/////////////////////////////////////////////////////////////////////////////////////

// Cloud STT using TheCubeServer
class RemoteTranscriber : public I_Transcriber, public I_RemoteApi {
public:
    RemoteTranscriber();
    ~RemoteTranscriber();
    std::string transcribeBuffer(const uint16_t* audio, size_t length) override;
    std::string transcribeStream(const uint16_t* audio, size_t bufSize) override;

private:
    bool initTranscribing();
    bool streamAudio();
    bool stopTranscribing();
};

}
