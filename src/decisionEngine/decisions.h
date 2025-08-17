/*
██████╗ ███████╗ ██████╗██╗███████╗██╗ ██████╗ ███╗   ██╗███████╗   ██╗  ██╗
██╔══██╗██╔════╝██╔════╝██║██╔════╝██║██╔═══██╗████╗  ██║██╔════╝   ██║  ██║
██║  ██║█████╗  ██║     ██║███████╗██║██║   ██║██╔██╗ ██║███████╗   ███████║
██║  ██║██╔══╝  ██║     ██║╚════██║██║██║   ██║██║╚██╗██║╚════██║   ██╔══██║
██████╔╝███████╗╚██████╗██║███████║██║╚██████╔╝██║ ╚████║███████║██╗██║  ██║
╚═════╝ ╚══════╝ ╚═════╝╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═╝╚═╝  ╚═╝
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

// DecisionEngineMain: high-level composition root for recognition, scheduling,
// and execution. Wires together audio ingestion, transcription (local/remote),
// intent recognition, the scheduler, and triggers.
//
// Responsibilities
// - Orchestrate wakeword → transcription → intent recognition → execution
// - Route audio into transcribers (LocalTranscriber or RemoteTranscriber)
// - Choose recognition strategy (LocalIntentRecognition or RemoteIntentRecognition)
// - Own a Scheduler and TriggerManager for time/event-driven behaviors
// - Provide helper utilities (e.g., LLM-based response rewording)
#pragma once
#ifndef DECISIONS_H
#define DECISIONS_H

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
#include "../api/autoRegister.h"
#include "../audio/audioManager.h"
#include "../threadsafeQueue.h"
#include "globalSettings.h"
#include "httplib.h"
#include "personalityManager.h"
#include "intentRegistry.h"
#include "functionRegistry.h"
#include "scheduler.h"
#include "triggers.h"
// #include "decisionsError.hpp"
#include "remoteApi.h"
#include "transcriber.h"




namespace DecisionEngine {

class DecisionEngineMain : public I_AudioQueue {
public:
    DecisionEngineMain();
    ~DecisionEngineMain();
    void start();   // Start background workers and services
    void stop();    // Stop workers and release resources
    void restart(); // Convenience: stop + start
    void pause();   // Pause runtime activities where supported
    void resume();  // Resume after pause

private:
    std::shared_ptr<I_IntentRecognition> intentRecognition;
    std::shared_ptr<IntentRegistry> intentRegistry;
    std::shared_ptr<FunctionRegistry> functionRegistry;
    std::shared_ptr<I_Transcriber> transcriber;
    std::shared_ptr<Scheduler> scheduler;
    std::shared_ptr<TriggerManager> triggerManager;
    std::shared_ptr<Personality::PersonalityManager> personalityManager;
    std::shared_ptr<TheCubeServer::TheCubeServerAPI> remoteServerAPI;
};

/////////////////////////////////////////////////////////////////////////////////////

std::vector<IntentCTorParams> getSystemIntents();
std::vector<IntentCTorParams> getSystemSchedule();

// Helper: Build an emotionally nuanced response using LLM output. The prompt
// is generated from input text and EmotionSimple values, and submitted to the
// remote server. The future resolves to a JSON string containing the response.
std::future<std::string> modifyStringUsingAIForEmotionalState(const std::string& input, const std::vector<Personality::EmotionSimple>& emotions, const std::shared_ptr<TheCubeServer::TheCubeServerAPI>& remoteServerAPI, const std::function<void(std::string)>& progressCB);

}; // namespace DecisionEngine

#endif // DECISIONS_H
