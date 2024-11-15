/*

The decision engine will be responsible for making decisions based on the intent of the user.
It will provide API endpoints for other apps on the system to interact with it.
When the speech in class detects a wake word, it will route audio into the decision engine 
which will then pass the audio to the whisper service for translation to text. If the user
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

// class to interact with open AI

