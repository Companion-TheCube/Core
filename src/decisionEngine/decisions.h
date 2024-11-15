#ifndef DECISIONS_H
#define DECISIONS_H

#include "../database/cubeDB.h"
#include <logger.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include "nlohmann/json.hpp"

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
    using Parameters = std::unordered_map<std::string, std::string>;
    using Action = std::function<void(const Parameters&)>;

    Intent(const std::string& intentName, const Action& action);
    Intent(const std::string& intentName, const Action& action, const Parameters& parameters);

    const std::string& getIntentName() const;
    const Parameters& getParameters() const;

    void setParameters(const Parameters& parameters);
    void addParameter(const std::string& key, const std::string& value);

    void execute() const;

    const std::string serialize();
    static std::shared_ptr<Intent> deserialize(const std::string& serializedIntent);
    void setSerializedData(const std::string& serializedData);

private:
    std::string intentName;
    Action action;
    Parameters parameters;
};

}; // namespace DecisionEngine

#endif // DECISIONS_H
