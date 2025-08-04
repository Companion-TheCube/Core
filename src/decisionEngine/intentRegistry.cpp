#include "intentRegistry.h"

namespace DecisionEngine {
// Intent - class that contains the intent data including the action to take

/**
 * @brief Construct a new Intent object with no parameters
 * briefDesc and responseString will be set to empty strings. Call setBriefDesc and setResponseString to set these values. *
 * @param intentName
 * @param action
 */
Intent::Intent(const std::string& intentName, const Action& action)
{
    this->intentName = intentName;
    this->action = action;
    this->briefDesc = "";
    this->responseString = "";
    this->type = Intent::IntentType::COMMAND;
}

/**
 * @brief Construct a new Intent object
 * briefDesc and responseString will be set to empty strings. Call setBriefDesc and setResponseString to set these values. *
 * @param intentName
 * @param action
 * @param parameters
 */
Intent::Intent(const std::string& intentName, const Action& action, const Parameters& parameters)
{
    this->intentName = intentName;
    this->action = action;
    this->parameters = parameters;
    this->briefDesc = "";
    this->responseString = "";
    this->type = Intent::IntentType::COMMAND;
}

/**
 * @brief Construct a new Intent object
 *
 * @param intentName The name of the intent
 * @param action The action to take when the intent is executed
 * @param parameters The parameters for the intent. For example, "What time is it?" would have a parameter of "time" with the value of the current time.
 * @param briefDesc A brief description of the intent. For example, "Get the current time"
 * @param responseString This is the string that will be returned to the user when the intent is executed.
 * This string can contain placeholders for the parameters. The placeholders will be replaced with the actual values.
 * The placeholders should be in the format ${parameterName}
 */
Intent::Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString)
{
    this->intentName = intentName;
    this->action = action;
    this->parameters = parameters;
    this->briefDesc = briefDesc;
    this->responseString = responseString;
    this->type = Intent::IntentType::COMMAND;
}

/**
 * @brief Construct a new Intent object
 *
 * @param intentName The name of the intent
 * @param action The action to take when the intent is executed
 * @param parameters The parameters for the intent. For example, "What time is it?" would have a parameter of "time" with the value of the current time.
 * @param briefDesc A brief description of the intent. For example, "Get the current time"
 * @param responseString The string that will be returned to the user when the intent is executed. This string can contain placeholders for the parameters. For example, "The current time is ${time}"
 * @param type The type of the intent. This can be either a question or a command.
 */
Intent::Intent(const std::string& intentName, const Action& action, const Parameters& parameters, const std::string& briefDesc, const std::string& responseString, Intent::IntentType type)
{
    this->intentName = intentName;
    this->action = action;
    this->parameters = parameters;
    this->briefDesc = briefDesc;
    this->responseString = responseString;
    this->type = type;
}

Intent::Intent(IntentCTorParams params)
{
    this->intentName = params.intentName;
    this->action = params.action;
    this->parameters = params.parameters;
    this->briefDesc = params.briefDesc;
    this->responseString = params.responseString;
    this->type = params.type;
}

const std::string& Intent::getIntentName() const
{
    return intentName;
}

const Parameters& Intent::getParameters() const
{
    return parameters;
}

void Intent::setParameters(const Parameters& parameters)
{
    this->parameters = parameters;
}

bool Intent::setParameter(const std::string& key, const std::string& value)
{
    if (parameters.find(key) != parameters.end()) {
        parameters[key] = value;
        return true;
    }
    return false;
}

void Intent::addParameter(const std::string& key, const std::string& value)
{
    parameters[key] = value;
}

void Intent::execute() const
{
    if (action) {
        // TODO: add TTS support
        action(parameters, *this);
    } else
        throw DecisionEngine::DecisionEngineError("No action set for intent: " + intentName, DecisionEngine::DecisionErrorType::NO_ACTION_SET);
}

const std::string Intent::serialize()
{
    nlohmann::json j;
    j["intentName"] = intentName;
    j["parameters"] = parameters;
    return j.dump();
}

/**
 * @brief Convert a JSON object to an Intent::Action. USed to convert a serialized intent to a callable action. *
 * @param action_json
 * @return Intent::Action
 */
Action convertJsonToAction(const nlohmann::json& action_json)
{
    // TODO: Implement this
    return nullptr;
}

std::shared_ptr<Intent> Intent::deserialize(const std::string& serializedIntent)
{
    // TODO: This needs checks to ensure properly formatted JSON
    nlohmann::json j = nlohmann::json::parse(serializedIntent);
    std::string intentName = j["intentName"];
    Parameters parameters = j["parameters"];
    Action action = convertJsonToAction(j["action"]);
    return std::make_shared<Intent>(intentName, action, parameters);
}

const std::string& Intent::getBriefDesc() const
{
    return briefDesc;
}

const std::string Intent::getResponseString() const
{
    std::string temp = responseString;
    if (responseStringScored.size() > 0 && personalityManager) {
        auto score = personalityManager->calculateEmotionalMatchScore(emotionRanges);
        auto scoreIndex = Personality::interpretScore(score);
        if (scoreIndex < responseStringScored.size())
            temp = responseStringScored[scoreIndex];
        else {
            CubeLog::warning("Score index out of range for intent: " + intentName);
        }
    }

    // Parse the response string and replace the placeholders with the actual values
    bool allParamsFound = true;
    for (auto& param : parameters) {
        std::string placeholder = "${" + param.first + "}";
        size_t pos = temp.find(placeholder);
        if (pos != std::string::npos)
            temp.replace(pos, placeholder.length(), param.second);
        else
            allParamsFound = false;
    }
    if (!allParamsFound)
        CubeLog::warning("Not all parameters found in response string for intent: " + intentName);
    return temp;
}

void Intent::setResponseString(const std::string& responseString)
{
    this->responseString = responseString;
}

void Intent::setScoredResponseStrings(const std::vector<std::string>& responseStrings)
{
    if (responseStrings.size() > 5) {
        CubeLog::warning("Too many scored response strings for intent: " + intentName);
        CubeLog::warning("Only the first 5 will be used.");
    }
    for (size_t i = 0; i < responseStrings.size() && i < 5; i++) {
        responseStringScored.push_back(responseStrings[i]);
    }
}

void Intent::setScoredResponseString(const std::string& responseString, int score)
{
    if (score < 0 || score > 5) {
        CubeLog::warning("Score out of range for intent: " + intentName);
        CubeLog::warning("Setting score to " + std::string(score < 0 ? "0" : "5"));
        score = score < 0 ? 0 : 5;
    }
    std::string t = "";
    if (responseStringScored.size() > 0)
        t = responseStringScored.at(responseStringScored.size() - 1);
    while (responseStringScored.size() <= score) {
        responseStringScored.push_back(t);
    }
    responseStringScored.at(score) = responseString;
}

void Intent::setEmotionalScoreRanges(const std::vector<Personality::EmotionRange>& emotionRanges)
{
    this->emotionRanges = emotionRanges;
}

void Intent::setEmotionScoreRange(const Personality::EmotionRange& emotionRange)
{
    auto emote = emotionRange.emotion;
    bool found = false;
    for (auto& range : emotionRanges) {
        if (range.emotion == emote) {
            range = emotionRange;
            found = true;
            break;
        }
    }
    if (!found) {
        CubeLog::warning("Emotion range not found for intent: " + intentName);
        CubeLog::warning("Adding new range for emotion: " + Personality::emotionToString(emote));
        emotionRanges.push_back(emotionRange);
    }
}

void Intent::setSerializedData(const std::string& serializedData)
{
    this->serializedData = serializedData;
}

const std::string& Intent::getSerializedData() const
{
    return serializedData;
}

void Intent::setBriefDesc(const std::string& briefDesc)
{
    this->briefDesc = briefDesc;
}

}