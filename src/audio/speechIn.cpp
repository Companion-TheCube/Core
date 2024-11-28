/*

In this file we will interact with the wake word detector to get wake word events.
We will also interact with the server API to:
    1. Translate speech to text (Whisper)
    2. Get the intent of the text (GPT4)
        a. Trigger the intent
        b. Get the response of the intent
    3. Provide feedback to the user in the form of on screen text and/or text to speech (Whisper)

The audioOutput.cpp file will need some sort static methods to allow classes in this file to interact with it.
Either that or we can have the relevant class from that file be instantiated as a shared pointer, and passed to the classes in this file.

OpenWakeWord will be used for wake word detection. It is a python script that the app manager will have to start. Audio will be sent to
that script via a named pipe. When a wake word is detected, the script will send a message back to the app manager via an http request.
We'll need to have an API endpoint that provides this functionality and triggers sending audio to the remote server.

When the wake word is detected, we'll need to tell the audioOutput class to play a sound to let the user know that the wake word was detected.

*/

#include "speechIn.h"

// Audio router
// This class will need a reference to the decision engine so that it can pass the audio to the it.
// There shall be a FIFO queue that will hold the audio data. When the decision engine is ready for audio,
// it will pull the audio from the queue. The audio router will have a method to add audio to the queue.
// The audio router will also have a method to start and stop the audio stream.
// The audio router will also have a method to get the number of audio samples in the queue.

// Wake word detector monitor (for making sure that the wake word detector is running)

SpeechIn::~SpeechIn()
{
    stop();
}

void SpeechIn::start()
{
}

void SpeechIn::stop()
{
}

std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> SpeechIn::getAudioDataQueue()
{
    return audioQueue;
}

std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> SpeechIn::getPreTriggerAudioDataQueue()
{
    return preTriggerAudioData;
}

size_t SpeechIn::getAudioDataSize()
{
    return 0;
}

size_t SpeechIn::getPreTriggerAudioDataSize()
{
    return 0;
}

void SpeechIn::clearAudioData()
{
}

void SpeechIn::clearPreTriggerAudioData()
{
}

size_t SpeechIn::getFifoSize()
{
    return 0;
}

size_t SpeechIn::getPreTriggerFifoSize()
{
    return 0;
}

unsigned int SpeechIn::getSampleRate()
{
    return sampleRate;
}

unsigned int SpeechIn::getNumChannels()
{
    return numChannels;
}

unsigned int SpeechIn::getBitsPerSample()
{
    return bitsPerSample;
}

unsigned int SpeechIn::getBytesPerSample()
{
    return bytesPerSample;
}

unsigned int SpeechIn::getNumSamples()
{
    return numSamples;
}

unsigned int SpeechIn::getNumBytes()
{
    return numSamples * bytesPerSample;
}

void SpeechIn::audioInputThread()
{
}

void SpeechIn::writeAudioDataToSocket(int16_t* buffer, size_t bufferSize)
{
}