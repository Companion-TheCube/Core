/*
██╗███╗   ██╗████████╗███████╗███╗   ██╗████████╗██████╗ ███████╗ ██████╗ ██╗███████╗████████╗██████╗ ██╗   ██╗██╗  ██╗
██║████╗  ██║╚══██╔══╝██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔══██╗╚██╗ ██╔╝██║  ██║
██║██╔██╗ ██║   ██║   █████╗  ██╔██╗ ██║   ██║   ██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   ██████╔╝ ╚████╔╝ ███████║
██║██║╚██╗██║   ██║   ██╔══╝  ██║╚██╗██║   ██║   ██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══██╗  ╚██╔╝  ██╔══██║
██║██║ ╚████║   ██║   ███████╗██║ ╚████║   ██║   ██║  ██║███████╗╚██████╔╝██║███████║   ██║   ██║  ██║   ██║██╗██║  ██║
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝╚═╝╚═╝  ╚═╝
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


// IntentRegistry and Recognition: hold intents and determine which intent best
// matches a user utterance.
//
// Components
// - Intent: name + action + parameters + response strings (optionally emotion-scored)
// - IntentRegistry: in-memory map of intents, plus HTTP API surface via AutoRegisterAPI
// - I_IntentRecognition: strategy interface for recognizing intents
// - RemoteIntentRecognition: defers recognition to TheCubeServer (WIP)
//
// Typical flow
// 1) Intents are registered (system + app-provided)
// 2) Recognition strategy analyzes text and selects a matching Intent
// 3) The chosen Intent::execute() performs the action and response composition
#pragma once
#ifndef INTENT_REGISTRY_H
#define INTENT_REGISTRY_H
#include "utils.h"
#ifndef LOGGER_H
#include <logger.h>
#endif
#ifndef API_I_H
#include "../api/api.h"
#endif
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
#include "functionRegistry.h"
// #include "decisionsError.hpp"
#include "remoteApi.h"

namespace DecisionEngine {

class Intent;
struct IntentCTorParams;

using Parameters = std::unordered_map<std::string, std::string>;
using Action = std::function<void(const Parameters&, Intent)>;
using TimePoint = std::chrono::system_clock::time_point;

class Intent {
public:
    // TODO: add a mutex so that the calling of execute can be thread safe
    enum class IntentType {
        QUESTION,
        COMMAND
    } type;

    /**
     * Parameters: key-value data passed to the action. Keys can be referenced in
     * response strings as ${key} and replaced during Intent::getResponseString().
     */

    Intent() {};
    Intent(const std::string& intentName, const Action& action);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString, Intent::IntentType type);
    Intent(IntentCTorParams params);

    const std::string& getIntentName() const;
    const Parameters& getParameters() const;

    void setParameters(const Parameters& parameters);
    bool setParameter(const std::string& key, const std::string& value);
    void addParameter(const std::string& key, const std::string& value);

    void execute() const;

    const std::string serialize();
    static std::shared_ptr<Intent> deserialize(const std::string& serializedIntent);
    void setSerializedData(const std::string& serializedData);
    const std::string& getSerializedData() const;

    const std::string& getBriefDesc() const;
    void setBriefDesc(const std::string& briefDesc);
    const std::string getResponseString() const;
    void setResponseString(const std::string& responseString);
    void setScoredResponseStrings(const std::vector<std::string>& responseStrings);
    void setScoredResponseString(const std::string& responseString, int score);

    void setEmotionalScoreRanges(const std::vector<Personality::EmotionRange>& emotionRanges);
    void setEmotionScoreRange(const Personality::EmotionRange& emotionRange);

    // Function/Capability helpers. These forward to an attached FunctionRegistry
    void runFunctionAsync(const std::string& functionName, const nlohmann::json& args, std::function<void(const nlohmann::json&)> onComplete = nullptr) const;
    nlohmann::json runFunctionSync(const std::string& functionName, const nlohmann::json& args, uint32_t timeoutMs = 4000) const;
    void runCapabilityAsync(const std::string& capabilityName, const nlohmann::json& args, std::function<void(const nlohmann::json&)> onComplete = nullptr) const;
    nlohmann::json runCapabilitySync(const std::string& capabilityName, const nlohmann::json& args, uint32_t timeoutMs = 4000) const;

    const std::vector<std::string> getMatchingParams();
    void setMatchingParams(const std::vector<std::string>& matchingParams);
    const std::vector<std::string> getMatchingPhrases();
    void setMatchingPhrases(const std::vector<std::string>& matchingPhrases);

private:
    std::string intentName;
    Action action;
    Parameters parameters;
    std::string briefDesc;
    /**
     * Response string returned to the user when the intent is executed. May include
     * parameter placeholders in the form ${parameterName}, substituted at render time.
     */
    std::string responseString = "";
    std::string serializedData = "";
    /**
     * Emotion-scored variants of responseString. Index selection is driven by
     * PersonalityManager score bucketing. If empty or out-of-range, fall back to responseString.
     * Vector length: 0..5 (inclusive).
     */
    std::vector<std::string> responseStringScored = {};
    std::shared_ptr<Personality::PersonalityManager> personalityManager;

    // Optional FunctionRegistry used by Intent helper methods
    std::weak_ptr<FunctionRegistry> functionRegistry;
public:
    void setFunctionRegistry(std::shared_ptr<FunctionRegistry> registry) { functionRegistry = registry; }

    std::vector<Personality::EmotionRange> emotionRanges;

    std::vector<std::string> matchingParams;
    std::vector<std::string> matchingPhrases;
};

/////////////////////////////////////////////////////////////////////////////////////

struct IntentCTorParams {
    IntentCTorParams() {};
    std::string intentName = "";
    Action action = nullptr;
    Parameters parameters = Parameters();
    std::string briefDesc = "";
    std::string responseString = "";
    Intent::IntentType type = Intent::IntentType::COMMAND;
};

/////////////////////////////////////////////////////////////////////////////////////

class IntentRegistry : public AutoRegisterAPI<IntentRegistry> {
public:
    IntentRegistry();
    bool registerIntent(const std::string& intentName, const std::shared_ptr<Intent> intent);
    bool unregisterIntent(const std::string& intentName);
    std::shared_ptr<Intent> getIntent(const std::string& intentName);
    std::vector<std::string> getIntentNames();
    std::vector<std::shared_ptr<Intent>> getRegisteredIntents();
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;
    void setFunctionRegistry(std::shared_ptr<FunctionRegistry> registry);

private:
    /**
     * @brief Map of intent names to intents
     */
    std::unordered_map<std::string, std::shared_ptr<Intent>> intentMap;
    std::shared_ptr<FunctionRegistry> functionRegistry;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_IntentRecognition {
public:
    virtual ~I_IntentRecognition() = default;
    virtual std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) = 0;
    virtual bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) = 0;
    virtual bool recognizeIntentAsync(const std::string& intentString) = 0;
    std::shared_ptr<IntentRegistry> intentRegistry;
};

/////////////////////////////////////////////////////////////////////////////////////

class RemoteIntentRecognition : public I_IntentRecognition, public I_RemoteApi {
public:
    // Remote recognition: ships input text to TheCubeServer for LLM-backed matching.
    RemoteIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~RemoteIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;

private:
    std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) override;
};
/////////////////////////////////////////////////////////////////////////////////////

}
#endif // INTENT_REGISTRY_H
