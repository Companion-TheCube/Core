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
#include "decisionsError.hpp"
#include "interfaceDefs.hpp"

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
     * @brief Parameters for the intent
     * The parameters are key value pairs that are used to pass data to the action function.
     * The first string is the key and the second string is the value. The key can be used
     * in response string to insert the value. See responseString for more information.
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
     * @brief This is the string that will be returned to the user when the intent is executed. If TTS
     * is enabled, this string will be spoken to the user. This string can contain placeholders for the parameters.
     * The placeholders will be replaced with the actual values. The placeholders should be in the format ${parameterName}
     */
    std::string responseString = "";
    std::string serializedData = "";
    /**
     * @brief These strings should represent the response string at different emotional scores. The strings should be ordered from
     * closest to the emotional target to farthest from the emotional target. If the emotional score is out of range, the standard
     * response string will be used. Therefore, this vector can be 0 to 5 strings long, inclusive.
     */
    std::vector<std::string> responseStringScored = {};
    std::shared_ptr<Personality::PersonalityManager> personalityManager;

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

private:
    /**
     * @brief Map of intent names to intents
     */
    std::unordered_map<std::string, std::shared_ptr<Intent>> intentMap;
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

class LocalIntentRecognition : public I_IntentRecognition {
public:
    LocalIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~LocalIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;

private:
    std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) override;
    std::vector<std::jthread*> recognitionThreads;
    std::vector<std::shared_ptr<TaskQueueWithData<std::function<void()>, std::string>>> taskQueues;
    bool threadsReady = false;
    // Pattern matching
    // Weighted pattern matching
    // Machine learning?
};

/////////////////////////////////////////////////////////////////////////////////////

class RemoteIntentRecognition : public I_IntentRecognition, public I_RemoteApi {
public:
    RemoteIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~RemoteIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;

private:
    std::shared_ptr<Intent> recognizeIntent(const std::string& name, const std::string& intentString) override;
    std::jthread* recognitionThread;
};
/////////////////////////////////////////////////////////////////////////////////////

}
#endif // INTENT_REGISTRY_H

