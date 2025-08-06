/*
██████╗ ███████╗ ██████╗██╗███████╗██╗ ██████╗ ███╗   ██╗███████╗    ██████╗██████╗ ██████╗ 
██╔══██╗██╔════╝██╔════╝██║██╔════╝██║██╔═══██╗████╗  ██║██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██║  ██║█████╗  ██║     ██║███████╗██║██║   ██║██╔██╗ ██║███████╗   ██║     ██████╔╝██████╔╝
██║  ██║██╔══╝  ██║     ██║╚════██║██║██║   ██║██║╚██╗██║╚════██║   ██║     ██╔═══╝ ██╔═══╝ 
██████╔╝███████╗╚██████╗██║███████║██║╚██████╔╝██║ ╚████║███████║██╗╚██████╗██║     ██║     
╚═════╝ ╚══════╝ ╚═════╝╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

The decision engine will be responsible for making decisions based on user commands, schedules,
triggers, and the personality of TheCube.
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
        scheduled based on the personality of TheCube. For instance, if the playfulness setting
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

    //
    this->audioQueue = AudioManager::audioInQueue;
    remoteServerAPI = std::make_shared<TheCubeServer::TheCubeServerAPI>(audioQueue);
    intentRegistry = std::make_shared<IntentRegistry>();

    bool remoteIntentRecognition = GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::REMOTE_INTENT_RECOGNITION_ENABLED);
    if (remoteIntentRecognition) {
        intentRecognition = std::make_shared<RemoteIntentRecognition>(intentRegistry);
        (std::dynamic_pointer_cast<RemoteIntentRecognition>(intentRecognition))->setRemoteServerAPIObject(remoteServerAPI);
    } else
        intentRecognition = std::make_shared<LocalIntentRecognition>(intentRegistry);
    intentRecognition = std::make_shared<LocalIntentRecognition>(intentRegistry);
    bool remoteTranscription = GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::REMOTE_TRANSCRIPTION_ENABLED);
    if (remoteTranscription) {
        transcriber = std::make_shared<RemoteTranscriber>();
        (std::dynamic_pointer_cast<RemoteTranscriber>(transcriber))->setRemoteServerAPIObject(remoteServerAPI);
    } else
        transcriber = std::make_shared<LocalTranscriber>();



    ///////////// Testing /////////////
    // TODO: remove this test code
    auto pMan = std::make_shared<Personality::PersonalityManager>();
    pMan->registerInterface();
    std::vector<Personality::EmotionRange> inputRange = {
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::CURIOSITY },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::PLAYFULNESS },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::EMPATHY },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::ASSERTIVENESS },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::ATTENTIVENESS },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::CAUTION },
        Personality::EmotionRange { 1, 1, 1.f, Personality::Emotion::EmotionType::ANNOYANCE }
    };
    auto score = pMan->calculateEmotionalMatchScore(inputRange);
    CubeLog::fatal("Emotional match score: " + std::to_string(score));

    
    for (size_t i = 0; i < 20; i++) {
        IntentCTorParams params;
        params.intentName = "Test Intent" + std::to_string(i);
        params.action = [i](const Parameters& params, Intent intent) {
            std::cout << "Test intent executed: " << std::to_string(i) << std::endl;
            intent.setParameter("TestParam", "--The new " + std::to_string(i) + " testValue--");
            CubeLog::fatal(intent.getResponseString());
        };
        params.parameters = Parameters({ { "TestParam", "testValue" } });
        params.briefDesc = "Test intent description: " + std::to_string(i);
        params.responseString = "Test intent response ${TestParam}";
        std::shared_ptr<Intent> testIntent = std::make_shared<Intent>(params);
        if (!intentRegistry->registerIntent("Test Intent: " + std::to_string(i), testIntent))
            CubeLog::error("Failed to register test intent" + std::to_string(i));
    }
    intentRecognition->recognizeIntentAsync("Do TestParam", [](std::shared_ptr<Intent> intent) {
        intent->execute();
    });
}

DecisionEngineMain::~DecisionEngineMain()
{
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO:
// Triggers need to work with the personalityManager so that they can be scheduled based on the personality of TheCube.
// Each trigger will need to have some definition of what personality settings it is looking for. For example, a trigger that
// plays a sound when the user returns to the desk might only be active if the playfulness setting is set to a certain level.
// Interface: Trigger - class that triggers intents. Stores a reference to whatever state it is monitoring.
// TimeBasedTrigger - class that triggers intents based on time. This will use the scheduler to schedule intents.
// EventBasedTrigger - class that triggers intents based on events like the user returning to the desk
// APITrigger - class that triggers intents based on API calls

bool I_Trigger::isEnabled() const
{
    return enabled;
}

void I_Trigger::setEnabled(bool enabled)
{
    this->enabled = enabled;
}

void I_Trigger::trigger()
{
    if (enabled)
        triggerFunction();
}

bool I_Trigger::getTriggerState() const
{
    return checkTrigger();
}

void I_Trigger::setTriggerFunction(std::function<void()> triggerFunction)
{
    this->triggerFunction = triggerFunction;
}

void I_Trigger::setCheckTrigger(std::function<bool()> checkTrigger)
{
    this->checkTrigger = checkTrigger;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Modify a string to align with a given emotional state.
 *
 * @param input The string to modify
 * @param emotions The emotional state to align with
 * @param remoteServerAPI The remote server API object
 * @param progressCB The callback function to call with progress updates. Will be called on each of the response fragments from the LLM.
 * @return std::future<std::string> A future that will yield a JSON string with the modified string under the key "output". May contain other keys.
 */
std::future<std::string> DecisionEngine::modifyStringUsingAIForEmotionalState(const std::string& input, const std::vector<Personality::EmotionSimple>& emotions, const std::shared_ptr<TheCubeServer::TheCubeServerAPI>& remoteServerAPI, const std::function<void(std::string)>& progressCB)
{
    static const std::string chatString = "I have a robot program with an emotional state defined by 7 qualities: curiosity, playfulness, empathy, assertiveness, attentiveness, caution, and annoyance. Each quality has a value between 1 and 100 (inclusive). The robot interacts with the user by speaking sentences, and I need you to revise a given string to align with its emotional state.\n\n"
                                          "Here are the definitions of each quality:\n"
                                          "Curiosity (1-100): Reflects the robot’s interest in exploring or learning. High values make it more inquisitive or engaged.\n"
                                          "Playfulness (1-100): Reflects the robot's tendency to be lighthearted and fun. High values lead to a more cheerful and humorous tone.\n"
                                          "Empathy (1-100): Reflects the robot's capacity for understanding and kindness. High values result in more compassionate or sensitive language.\n"
                                          "Assertiveness (1-100): Reflects the robot's directness and determination. High values lead to more confident and action-oriented language.\n"
                                          "Attentiveness (1-100): Reflects the robot's focus and precision. High values make it more detailed and thorough.\n"
                                          "Caution (1-100): Reflects the robot's tendency to warn or hesitate. High values make it more reserved or cautious.\n"
                                          "Annoyance (1-100): Reflects the robot’s irritation level. High values lead to more curt or sarcastic language.\n\n"
                                          "When I provide a string, the robot's emotional state will also be provided as a list of 7 values, each corresponding to the qualities above. Modify the string to align with these values while keeping the robot’s response functional and understandable.\n\n"
                                          "Example:\n\n"
                                          "Input:\n"
                                          "String: \"You have eight items on your to-do list today.\"\n"
                                          "Emotional State: {Curiosity: 40, Playfulness: 20, Empathy: 50, Assertiveness: 90, Attentiveness: 80, Caution: 30, Annoyance: 10}\n\n"
                                          "Output:\n"
                                          "\"You have eight items on your to-do list that must be accomplished today.\"\n\n"
                                          "Another Example:\n\n"
                                          "Input:\n"
                                          "String: \"You have eight items on your to-do list today.\"\n"
                                          "Emotional State: {Curiosity: 70, Playfulness: 60, Empathy: 80, Assertiveness: 20, Attentiveness: 50, Caution: 40, Annoyance: 10}\n\n"
                                          "Output:\n"
                                          "\"You've got eight items on your to-do list today. Let me know if you'd like help organizing them!\"\n\n"
                                          "Request:\n"
                                          "Modify the following string to align with the given emotional state. Use the emotional state values to adjust tone, word choice, and phrasing appropriately while retaining the core message.  Your response should be a JSON object with the key \"output\" and a value that is only the revised string. Other keys are permissible.\n\n"
                                          "Input:\n";
    std::string inputString = "String: \"" + input + "\"\nEmotional State: {";
    for (size_t i = 0; i < emotions.size(); i++) {
        inputString += Personality::emotionToString(emotions[i].emotion) + ": " + std::to_string(emotions[i].value);
        if (i < emotions.size() - 1)
            inputString += ", ";
    }
    inputString += "}";
    return remoteServerAPI->getChatResponseAsync(chatString + inputString, progressCB);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////