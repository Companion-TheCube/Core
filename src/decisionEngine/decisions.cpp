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

// IntentRouter - class that routes intents to the appropriate class or app

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
