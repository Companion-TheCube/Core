#ifndef DECISIONS_H
#define DECISIONS_H

#include "../database/cubeDB.h"
#include <logger.h>
#include "utils.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include <thread>
#include "nlohmann/json.hpp"
#include "whisper.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#define LOCAL_INTENT_RECOGNITION_THREAD_COUNT 4
#define LOCAL_INTENT_RECOGNITION_THREAD_SLEEP_MS 100

namespace DecisionEngine {
class Intent;
struct IntentCTorParams;

using Parameters = std::unordered_map<std::string, std::string>;
using Action = std::function<void(const Parameters&, Intent)>;

enum class DecisionErrorType {
    ERROR_NONE,
    INVALID_PARAMS,
    INTERNAL_ERROR,
    NO_ACTION_SET,
    UNKNOWN_ERROR
};

/////////////////////////////////////////////////////////////////////////////////////

class DecisionEngineError : public std::runtime_error {
    static uint32_t errorCount;
    DecisionErrorType errorType;
public:
    DecisionEngineError(const std::string& message, DecisionErrorType errorType = DecisionErrorType::UNKNOWN_ERROR);
};

/////////////////////////////////////////////////////////////////////////////////////

class Intent{
public:
    // TODO: add a mutex so that the calling of execute can be thread safe
    enum class IntentType {
        QUESTION,
        COMMAND
    }type;

    /**
     * @brief Parameters for the intent
     * The parameters are key value pairs that are used to pass data to the action function. 
     * The first string is the key and the second string is the value. The key can be used
     * in response string to insert the value. See responseString for more information.
     */

    Intent(){};
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
    const std::string getResponseString()const;
    void setResponseString(const std::string& responseString);
    void setBriefDesc(const std::string& briefDesc);

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
    std::string responseString;
    std::string serializedData;
};

/////////////////////////////////////////////////////////////////////////////////////

struct IntentCTorParams{
    IntentCTorParams(){};
    std::string intentName = "";
    Action action = nullptr;
    Parameters parameters = Parameters();
    std::string briefDesc = "";
    std::string responseString = "";
    Intent::IntentType type = Intent::IntentType::COMMAND;
};

/////////////////////////////////////////////////////////////////////////////////////

class IntentRegistry : public I_API_Interface{
public:
    IntentRegistry();
    bool registerIntent(const std::string& intentName, const std::shared_ptr<Intent> intent);
    bool unregisterIntent(const std::string& intentName);
    std::shared_ptr<Intent> getIntent(const std::string& intentName);
    std::vector<std::string> getIntentNames();
    std::vector<std::shared_ptr<Intent>>  getRegisteredIntents();
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    std::string getInterfaceName() const override;
private:
    /**
     * @brief Map of intent names to intents
     */
    std::unordered_map<std::string, std::shared_ptr<Intent>> intentMap;
};

/////////////////////////////////////////////////////////////////////////////////////

class Whisper{
public:
    Whisper();
    std::string transcribe(const std::string& audio);
private:
    std::jthread transcriberThread;
};

/////////////////////////////////////////////////////////////////////////////////////

class TheCubeServerAPI{
    
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

class LocalIntentRecognition : public I_IntentRecognition{
public:
    LocalIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~LocalIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;
private:
    std::shared_ptr<Intent> recognizeIntent(const std::string&  name, const std::string& intentString) override;
    std::vector<std::jthread*> recognitionThreads;
    std::vector<std::shared_ptr<TaskQueueWithData<std::function<void()>, std::string>>> taskQueues;
    bool threadsReady = false;
    // Pattern matching
    // Weighted pattern matching
    // Machine learning?
};

/////////////////////////////////////////////////////////////////////////////////////

class RemoteIntentRecognition : public I_IntentRecognition{
public:
    RemoteIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    ~RemoteIntentRecognition();
    bool recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback) override;
    bool recognizeIntentAsync(const std::string& intentString) override;
private:
    std::shared_ptr<Intent> recognizeIntent(const std::string&  name, const std::string& intentString) override;
    std::jthread* recognitionThread;
    httplib::Client cli;
};

/////////////////////////////////////////////////////////////////////////////////////

class DecisionEngineMain{
public:
    DecisionEngineMain();
    ~DecisionEngineMain();
    void start();
    void stop();
    void restart();
    void pause();
    void resume();
    void setIntentRecognition(std::shared_ptr<I_IntentRecognition> intentRecognition);
    void setTranscriber(std::shared_ptr<Whisper> transcriber);
    void setIntentRegistry(std::shared_ptr<IntentRegistry> intentRegistry);
    void setAPIKey(const std::string& apiKey);
    void setAPIURL(const std::string& apiURL);
    void setAPIPort(const std::string& apiPort);
    void setAPIPath(const std::string& apiPath);

    std::shared_ptr<IntentRegistry> getIntentRegistry();
private:
    std::shared_ptr<I_IntentRecognition> intentRecognition;
    std::shared_ptr<Whisper> transcriber;
    std::shared_ptr<IntentRegistry> intentRegistry;
    std::string apiKey;
    std::string apiURL;
    std::string apiPort;
    std::string apiPath;
};

/////////////////////////////////////////////////////////////////////////////////////

std::vector<IntentCTorParams> getSystemIntents();

}; // namespace DecisionEngine

#endif// DECISIONS_H
