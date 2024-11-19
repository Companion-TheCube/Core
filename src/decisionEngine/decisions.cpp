/*
████████╗██╗  ██╗███████╗                                
╚══██╔══╝██║  ██║██╔════╝                                
   ██║   ███████║█████╗                                  
   ██║   ██╔══██║██╔══╝                                  
   ██║   ██║  ██║███████╗                                
   ╚═╝   ╚═╝  ╚═╝╚══════╝                                
██████╗ ███████╗ ██████╗██╗███████╗██╗ ██████╗ ███╗   ██╗
██╔══██╗██╔════╝██╔════╝██║██╔════╝██║██╔═══██╗████╗  ██║
██║  ██║█████╗  ██║     ██║███████╗██║██║   ██║██╔██╗ ██║
██║  ██║██╔══╝  ██║     ██║╚════██║██║██║   ██║██║╚██╗██║
██████╔╝███████╗╚██████╗██║███████║██║╚██████╔╝██║ ╚████║
╚═════╝ ╚══════╝ ╚═════╝╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝
███████╗███╗   ██╗ ██████╗ ██╗███╗   ██╗███████╗         
██╔════╝████╗  ██║██╔════╝ ██║████╗  ██║██╔════╝         
█████╗  ██╔██╗ ██║██║  ███╗██║██╔██╗ ██║█████╗           
██╔══╝  ██║╚██╗██║██║   ██║██║██║╚██╗██║██╔══╝           
███████╗██║ ╚████║╚██████╔╝██║██║ ╚████║███████╗         
╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝╚═╝  ╚═══╝╚══════╝         

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


// DecisionEngine - Main class that connects all the other classes together - this will need to connect to the personalityManager.

// TheCubeServerAPI - class to interact with TheCube Server API. Will use API key stored in CubeDB. Key is stored encrypted and will be decrypted at load time.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
}

/**
 * @brief Construct a new Intent object
 * 
 * @param intentName 
 * @param action 
 * @param parameters 
 * @param briefDesc 
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

/**
 * @brief Convert a JSON object to an Intent::Action. USed to convert a serialized intent to a callable action. * 
 * @param action_json 
 * @return Intent::Action 
 */
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

const std::string& Intent::getBriefDesc() const
{
    return briefDesc;
}

const std::string Intent::getResponseString()
{
    // Parse the response string and replace the placeholders with the actual values
    std::string temp = responseString;
    bool allParamsFound = true;
    for(auto& param : parameters)
    {
        std::string placeholder = "${" + param.first + "}";
        size_t pos = temp.find(placeholder);
        if(pos != std::string::npos) temp.replace(pos, placeholder.length(), param.second);
        else allParamsFound = false;
    }
    if(!allParamsFound) CubeLog::warning("Not all parameters found in response string for intent: " + intentName);
    return temp;
}

void Intent::setResponseString(const std::string& responseString)
{
    this->responseString = responseString;
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper.cpp library
// RemoteTranscriber - class that interacts with the TheCube Server API

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interface: IntentRecognition - class that determines the intent of the user
// LocalIntentRecognition - class that determines the intent of the user
// RemoteIntentRecognition - class that converts intent to action using the TheCube Server API

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IntentExecutor - class that converts intent to action locally

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IntentRegistry - class that contains the intents that are registered with the system
// Need to have Http endpoints for the API that allow other apps to register with the decision engine
IntentRegistry::IntentRegistry()
{
    // Register built in intents
    // registerIntent("test", Intent("test", [](const Intent::Parameters& params) { CubeLog::info("Test intent executed"); }));
    httplib::Client cli("https://dummyjson.com:80/");
    auto res = cli.Get("test");
    if(res && res->status == 200){
        auto text = res->body;
        CubeLog::info(text);
    }else{
        CubeLog::error("Error getting test data");
    }
}

bool IntentRegistry::registerIntent(const std::string& intentName, const Intent& intent)
{
    // Check if the intent is already registered
    if(intentMap.find(intentName) != intentMap.end()) return false;
    intentMap[intentName] = intent;
    return true;
}

bool IntentRegistry::unregisterIntent(const std::string& intentName)
{
    // Check if the intent is registered
    if(intentMap.find(intentName) == intentMap.end()) return false;
    intentMap.erase(intentName);
    return true;
}

std::shared_ptr<Intent> IntentRegistry::getIntent(const std::string& intentName)
{
    if(intentMap.find(intentName) == intentMap.end()) return nullptr;
    return std::make_shared<Intent>(intentMap[intentName]);
}

std::vector<std::string> IntentRegistry::getIntentNames()
{
    std::vector<std::string> intentNames;
    for(auto& intent : intentMap) intentNames.push_back(intent.first);
    return intentNames;
}

HttpEndPointData_t IntentRegistry::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // TODO: 
    // 1. Register an Intent
    // 2. Unregister an Intent
    // 3. Get an Intent
    // 4. Get all Intent Names
    return data;
}

std::string IntentRegistry::getInterfaceName() const
{
    return "IntentRegistry";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// BuiltInIntents - This should be a function or data structure that contains all the built in intents for the system. IntentRegistry
// constructor should call this function to register all the built in intents.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Scheduler - class that schedules intents. This class will be responsible for storing triggers and references to the intents that 
// they trigger. It will also be responsible for monitoring the states of the triggers and triggering the intents when the triggers are activated.
// This class will need to have a thread that runs in the background to monitor the triggers.

// Interface: Trigger - class that triggers intents. Stores a reference to whatever state it is monitoring.
// TimeBasedTrigger - class that triggers intents based on time
// EventBasedTrigger - class that triggers intents based on events

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t DecisionEngineError::errorCount = 0;
// DecisionError - class that extends std::runtime_error and contains the error message
DecisionEngineError::DecisionEngineError(const std::string& message, DecisionErrorType errorType) : std::runtime_error(message)
{
    this->errorType = errorType;
    CubeLog::error("DecisionEngineError: " + message);
    errorCount++;
}

/// Just testing that we can compile with whisper stuff
void whisperLoop(){
    struct whisper_context_params cparams = whisper_context_default_params();
    auto ctx = whisper_init_from_file_with_params("whisper-en-us-tdnn", cparams);
    
}