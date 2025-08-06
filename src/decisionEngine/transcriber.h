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


class I_Transcriber: public I_AudioQueue {
public:
    virtual ~I_Transcriber() = default;
    virtual std::string transcribeBuffer(const uint16_t* audio, size_t length) = 0;
    virtual std::string transcribeStream(const uint16_t* audio, size_t bufSize) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

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