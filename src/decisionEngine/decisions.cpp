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
DecisionEngineMain::DecisionEngineMain()
{
    /*
    TODO: 
    - Set up the connection to the speechIn class
    - Set up the connection to the TheCube Server API
    - Set up the connection to the personalityManager
    - Set up the scheduler
        - Scheduler needs to interface with the personalityManager so that certain things can be
        scheduled based on the personality of the user. For instance, if the playfulness setting
        is set sort of high, the scheduler might schedule some sort of animation or something to
        welcome the user back to their desk.

    The Process:
    - Once the wakeword is triggered, start moving audio from the speechIn class to the transcriber
    - If the user has remote transcription enabled:
        - The CubeServerAPI will then send the audio to the remote server for transcription
    - If the user does not have remote transcription enabled
        - The transcriber will use the whisper class to transcribe the audio
    - then we send the transcription to the intentRecognition class.
    - The intentRecognition class will then determine the intent of the user 
        - if remote intent detection is enabled, intentRecognition will send the transcription
        to the remote server for intent detection
        - if remote intent detection is not enabled, intentRecognition will use the local intent detection
    - The intentRecognition class will then return the intent to the DecisionEngineMain class
    */
    // transcriber = std::make_shared<Whisper>();
    intentRegistry = std::make_shared<IntentRegistry>();
    // TODO: we should load the correct intent recognition class based on the user's settings
    // intentRecognition = std::make_shared<RemoteIntentRecognition>(intentRegistry);
    // - if remoteIntentRecognition is enabled, pass the serverAPI object to the intentRecognition
    // (std::dynamic_pointer_cast<RemoteIntentRecognition>(intentRecognition))->setRemoteServerAPIObject(remoteServerAPI);
    intentRecognition = std::make_shared<LocalIntentRecognition>(intentRegistry);
    // TODO: load the correct transcriber based on the user's settings
    // transcriber = std::make_shared<LocalTranscriber>();
    // transcriber = std::make_shared<RemoteTranscriber>();
    // - if remoteTranscription is enabled, pass the serverAPI object to the transcriber
    // (std::dynamic_pointer_cast<RemoteTranscriber>(transcriber))->setRemoteServerAPIObject(remoteServerAPI);



    // TODO: remove this test code
    for(size_t i = 0; i < 20; i++){
        IntentCTorParams params;
        params.intentName = "Test Intent" + std::to_string(i);
        params.action = [i](const Parameters& params, Intent intent) {
            std::cout << "Test intent executed: " << std::to_string(i) << std::endl;
            intent.setParameter("TestParam", "--The new " + std::to_string(i) + " testValue--");
            CubeLog::fatal(intent.getResponseString());
        };
        params.parameters = Parameters({
            {"TestParam", "testValue"}
        });
        params.briefDesc = "Test intent description: " + std::to_string(i);
        params.responseString = "Test intent response ${TestParam}";
        std::shared_ptr<Intent> testIntent = std::make_shared<Intent>(params);
        if(!intentRegistry->registerIntent("Test Intent: " + std::to_string(i), testIntent))
            CubeLog::error("Failed to register test intent" + std::to_string(i));
    }
    intentRecognition->recognizeIntentAsync("Do TestParam", [](std::shared_ptr<Intent> intent) {
        intent->execute();
    });
}

DecisionEngineMain::~DecisionEngineMain()
{
}

const std::shared_ptr<IntentRegistry> DecisionEngineMain::getIntentRegistry()const
{
    return intentRegistry;
}

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
    if(parameters.find(key) != parameters.end()){
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
    if(action){
        // TODO: add TTS support
        action(parameters, *this);
    }
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
    // TODO: The returned string needs to be run through the personalityManager to get the final response string
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
// TODO:
// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper class. Whisper class should be initialized and a reference to it saved for future use.
// RemoteTranscriber - class that interacts with the TheCube Server API.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interface: IntentRecognition - class that determines the intent of the user
// LocalIntentRecognition - class that determines the intent of the user without the need of an LLM. Use a score based pattern matching system.
LocalIntentRecognition::LocalIntentRecognition(std::shared_ptr<IntentRegistry> intentRegistry)
{
    this->intentRegistry = intentRegistry;
    // create the recognition threads
    for(size_t i = 0; i < LOCAL_INTENT_RECOGNITION_THREAD_COUNT; i++){
        taskQueues.push_back(std::shared_ptr<TaskQueueWithData<std::function<void()>, std::string>>(new TaskQueueWithData<std::function<void()>,std::string>()));
        recognitionThreads.push_back(new std::jthread([this, i](std::stop_token st){
            while(!st.stop_requested()){
                genericSleep(LOCAL_INTENT_RECOGNITION_THREAD_SLEEP_MS);
                while(taskQueues[i]->size() != 0){
                    auto task = taskQueues[i]->pop();
                    if(task){
                        task();
                    }
                }
            }
        }));
    }
    threadsReady = true;
}

std::shared_ptr<Intent> LocalIntentRecognition::recognizeIntent(const std::string& name,const std::string& intentString)
{
    // TODO: although this code works, we need to implement a more advanced pattern matching system and we need to 
    // have pattern matches that are weighted. For example, if the user says "What time is it?" we need to have a pattern
    // match for "What time is it?" and "What is the time?" and "What time is it now?" and "What is the time now?" and so on.
    // TODO: This function should somehow return a score for the match so that when multiple intents match, we can 
    // choose the one with the highest score.

    auto l_intent = intentRegistry->getIntent(name);
    // first we make a vector of all the intents param names
    std::vector<std::vector<std::string>> intentParamNames;
    std::vector<std::string> paramNames;
    for(auto& param : l_intent->getParameters()){
        paramNames.push_back(param.first);
    }
    intentParamNames.push_back(paramNames);
    // now we need to tokenize the intentString
    std::vector<std::string> tokens;
    std::regex splitOnRegex("[\\s,.]+");
    std::sregex_token_iterator iter(intentString.begin(), intentString.end(), splitOnRegex, -1);
    std::sregex_token_iterator end;
    while(iter != end){
        tokens.push_back(iter->str());
        ++iter;
    }
    // now we need to find the intent that matches the most tokens
    size_t maxMatchIndex = 0;
    size_t maxMatchCount = 0;
    for(size_t i = 0; i < intentParamNames.size(); i++){
        size_t matchCount = 0;
        for(auto& token : tokens){
            for(auto& paramName : intentParamNames[i]){
                if(token == paramName){
                    matchCount++;
                    break;
                }
            }
        }
        if(matchCount > maxMatchCount){
            maxMatchCount = matchCount;
            maxMatchIndex = i;
        }
    }
    if(maxMatchCount == 0) return nullptr;
    return l_intent;
}

bool LocalIntentRecognition::recognizeIntentAsync(const std::string& intentString)
{
    return this->recognizeIntentAsync(intentString, [](std::shared_ptr<Intent> intent){});
}

bool LocalIntentRecognition::recognizeIntentAsync(const std::string& intentString, std::function<void(std::shared_ptr<Intent>)> callback)
{
    if(!threadsReady) return false;
    for(auto name : intentRegistry->getIntentNames()){
        size_t minIndex = 0;
        size_t minSize = taskQueues[0]->size();
        for(size_t i = 1; i < taskQueues.size(); i++){
            if(taskQueues[i]->size() < minSize){
                minSize = taskQueues[i]->size();
                minIndex = i;
            }
        }
        taskQueues[minIndex]->push([this, intentString, callback, name](){
            auto intent = recognizeIntent(name, intentString);
            if(intent) callback(intent);
        }, intentString);
    }
    return true;
}

LocalIntentRecognition::~LocalIntentRecognition()
{
    for(auto& thread : recognitionThreads){
        delete thread;
    }
    // for(auto& token : stopTokens){
    //     delete token;
    // }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// RemoteIntentRecognition - class that converts intent to action using the TheCube Server API
// TODO:

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IntentRegistry - class that contains the intents that are registered with the system
// Need to have Http endpoints for the API that allow other apps to register with the decision engine
IntentRegistry::IntentRegistry()
{
    for(auto& intent : getSystemIntents()){
        auto newIntent = std::make_shared<Intent>(intent);
        if(!registerIntent(intent.intentName, newIntent))
            CubeLog::error("Failed to register intent: " + intent.intentName);
    }
    
    // TODO: remove this. Testing only.
    httplib::Client cli("https://dummyjson.com");
    auto res = cli.Get("/test");
    if(res && res->status == 200){
        auto text = res->body;
        CubeLog::fatal(text);
    }else{
        CubeLog::error("Error getting test data");
    }
}

bool IntentRegistry::registerIntent(const std::string& intentName, const std::shared_ptr<Intent> intent)
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
    return intentMap[intentName];
}

std::vector<std::string> IntentRegistry::getIntentNames()
{
    std::vector<std::string> intentNames;
    for(auto& intent : intentMap) intentNames.push_back(intent.first);
    return intentNames;
}

std::vector<std::shared_ptr<Intent>> IntentRegistry::getRegisteredIntents()
{
    auto intents = std::vector<std::shared_ptr<Intent>>();
    for(auto& intent : intentMap) intents.push_back(intent.second);
    return intents;
}

HttpEndPointData_t IntentRegistry::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // TODO: 
    // 1. Register an Intent - This will need to take a JSON string object that contains the intent data and pass it to the deserialize function of the Intent class.
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

/**
 * @brief Get all the built-in system intents
 * @return std::vector<IntentCTorParams> 
 */
std::vector<IntentCTorParams> DecisionEngine::getSystemIntents()
{
    std::vector<IntentCTorParams> intents;
    // Test intent
    IntentCTorParams testIntent;
    testIntent.intentName = "Test Intent";
    testIntent.action = [](const Parameters& params, Intent intent) {
        CubeLog::info("Test intent executed");
    };
    testIntent.briefDesc = "Test intent description";
    testIntent.responseString = "Test intent response";
    testIntent.type = Intent::IntentType::COMMAND;
    intents.push_back(testIntent);
    // TODO: define all the system intents. this should include things like "What time is it?" and "What apps are installed?"
    // Should also include some pure command intents like "Do a flip" or "What was that last notification?"
    return intents;
}

std::vector<IntentCTorParams> DecisionEngine::getSystemSchedule()
{
    std::vector<IntentCTorParams> intents;
    // Test intent
    IntentCTorParams testIntent;
    testIntent.intentName = "Test Schedule";
    testIntent.action = [](const Parameters& params, Intent intent) {
        CubeLog::info("Test schedule executed");
    };
    testIntent.briefDesc = "Test schedule description";
    testIntent.responseString = "Test schedule response";
    testIntent.type = Intent::IntentType::COMMAND;
    intents.push_back(testIntent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO:
// Scheduler - class that schedules intents. This class will be responsible for storing triggers and references to the intents that 
// they trigger. It will also be responsible for monitoring the states of the triggers and triggering the intents when the triggers are activated.
// This class will need to have a thread that runs in the background to monitor the triggers.
// This class will also need to implement the API interface so that other apps can interact with it.
// TODO:
// Interface: Trigger - class that triggers intents. Stores a reference to whatever state it is monitoring.
// TimeBasedTrigger - class that triggers intents based on time
// EventBasedTrigger - class that triggers intents based on events like the user returning to the desk
// APITrigger - class that triggers intents based on API calls

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t DecisionEngineError::errorCount = 0;
DecisionEngineError::DecisionEngineError(const std::string& message, DecisionErrorType errorType) : std::runtime_error(message)
{
    this->errorType = errorType;
    CubeLog::error("DecisionEngineError: " + message);
    errorCount++;
}
