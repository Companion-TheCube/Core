/*

The decision engine will be responsible for making decisions based on the intent of the user.
It will provide API endpoints for other apps on the system to interact with it.
When the speech in class detects a wake word, it will route audio into the decision engine 
which will then pass the audio to the TheCube Server service for translation to text. If the user
has not configured the system to use the remote server, we may use the whisper.cpp library
to do the translation locally. 

Once the decision engine gets the text of the audio, it will then pass the text to the intent
detection service which will determine the intent of the user. The decision engine will then
use the response from the intent detection service to make a decision on what to do next.

To be evaluated: Using Whisper locally for users that don't want to pay for the service. 
Whisper.cpp (https://github.com/ggerganov/whisper.cpp) will be the best option for this, 
however it is not as accurate as the remote service and we'll have to implement our own intent detection.

*/


#include "decisions.h"

using namespace DecisionEngine;


// DecisionEngine - Main class that connects all the other classes together

// TheCubeServerAPI - class to interact with TheCube Server API. Will use API key stored in CubeDB. Key is stored encrypted and will be decrypted at load time.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Intent - class that contains the intent data including the action to take

Intent::Intent(const std::string& intentName, const Action& action)
{
    this->intentName = intentName;
    this->action = action;
}

Intent::Intent(const std::string& intentName, const Action& action, const Parameters& parameters)
{
    this->intentName = intentName;
    this->action = action;
    this->parameters = parameters;
}

const std::string& Intent::getIntentName() const
{
    return intentName;
}

const Intent::Parameters& Intent::getParameters() const
{
    return parameters;
}

void Intent::setParameters(const Parameters& parameters)
{
    this->parameters = parameters;
}

void Intent::addParameter(const std::string& key, const std::string& value)
{
    parameters[key] = value;
}

void Intent::execute() const
{
    if(action)
        action(parameters);
    else
        throw DecisionEngineError("No action set for intent: " + intentName, DecisionErrorType::NO_ACTION_SET);
}

const std::string Intent::serialize()
{
    nlohmann::json j;
    j["intentName"] = intentName;
    j["parameters"] = parameters;
    return j.dump();
}

Intent::Action convertJsonToAction(const nlohmann::json& action_json)
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
    Intent::Action action = convertJsonToAction(j["action"]);
    return std::make_shared<Intent>(intentName, action, parameters);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper.cpp library
// RemoteTranscriber - class that interacts with the TheCube Server API

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interface: IntentDetection - class that determines the intent of the user
// LocalIntentDetection - class that determines the intent of the user
// RemoteIntentRecognition - class that converts intent to action using the TheCube Server API

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interface: IntentRecognition - class that converts intent to action
// RemoteIntentRecognition - class that converts intent to action using the TheCube Server API
// LocalIntentRecognition - class that converts intent to action locally

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// BuiltInIntents - class that contains the built in intents for the system

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IntentRouter - class that routes intents to the appropriate class or app - Maybe don't Need?

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IntentRegistry - class that contains the intents that are registered with the system
// Need to have Http endpoints for the API that allow other apps to register with the decision engine

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t DecisionEngineError::errorCount = 0;
// DecisionError - class that extends std::runtime_error and contains the error message
DecisionEngineError::DecisionEngineError(const std::string& message, DecisionErrorType errorType) : std::runtime_error(message)
{
    this->errorType = errorType;
    CubeLog::error("DecisionEngineError: " + message);
    errorCount++;
}
