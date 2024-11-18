#ifndef DECISIONS_H
#define DECISIONS_H

#include "../database/cubeDB.h"
#include <logger.h>
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

namespace DecisionEngine {
enum class DecisionErrorType {
    ERROR_NONE,
    INVALID_PARAMS,
    INTERNAL_ERROR,
    NO_ACTION_SET,
    UNKNOWN_ERROR
};

class DecisionEngineError : public std::runtime_error {
    static uint32_t errorCount;
    DecisionErrorType errorType;
public:
    DecisionEngineError(const std::string& message, DecisionErrorType errorType = DecisionErrorType::UNKNOWN_ERROR);
};

class Intent{
    public:
    /**
     * @brief Parameters for the intent
     * The parameters are key value pairs that are used to pass data to the action function. 
     * The first string is the key and the second string is the value. The key can be used
     * in response string to insert the value. See responseString for more information.
     */
    using Parameters = std::unordered_map<std::string, std::string>;
    using Action = std::function<void(const Parameters&)>;

    Intent(){};
    Intent(const std::string& intentName, const Action& action);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString);

    const std::string& getIntentName() const;
    const Parameters& getParameters() const;

    void setParameters(const Parameters& parameters);
    void addParameter(const std::string& key, const std::string& value);

    void execute() const;

    const std::string serialize();
    static std::shared_ptr<Intent> deserialize(const std::string& serializedIntent);
    void setSerializedData(const std::string& serializedData);

    const std::string& getSerializedData() const;
    const std::string& getBriefDesc() const;
    const std::string getResponseString();
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

class IntentRegistry : public I_API_Interface{
public:
    IntentRegistry();
    bool registerIntent(const std::string& intentName, const Intent& intent);
    bool unregisterIntent(const std::string& intentName);
    std::shared_ptr<Intent> getIntent(const std::string& intentName);
    std::vector<std::string> getIntentNames();
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    std::string getInterfaceName() const override;
private:
    /**
     * @brief Map of intent names to intents
     */
    std::unordered_map<std::string, Intent> intentMap;
};

class Whisper{
public:
    Whisper();
    std::string transcribe(const std::string& audio);
private:
    std::jthread transcriberThread;
};

class I_IntentRecognition {
public:
    virtual ~I_IntentRecognition() = default;
    virtual std::shared_ptr<Intent> recognizeIntent(const std::string& intentString) = 0;
    std::shared_ptr<IntentRegistry> intentRegistry;
};

class LocalIntentRecognition : public I_IntentRecognition{
public:
    LocalIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry);
    std::shared_ptr<Intent> recognizeIntent(const std::string& intentString) override;
private:
    // Pattern matching
    // Weighted pattern matching
    // Machine learning?
};

}; // namespace DecisionEngine

#endif // DECISIONS_H
