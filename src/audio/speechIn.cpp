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

int audioInputCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData);

SpeechIn::~SpeechIn()
{
    stop();
}

void SpeechIn::start()
{
    // Start the audio input thread that handles rtaudio and streams audio to openwakeword
    this->audioInputThread = std::jthread([this](std::stop_token st) {
        this->audioInputThreadFn(st);
    });
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

void SpeechIn::audioInputThreadFn(std::stop_token st)
{
    // set up rt audio
    RtAudio audio;
    RtAudio::StreamParameters params;
    params.deviceId = audio.getDefaultInputDevice();
    params.nChannels = 1;
    params.firstChannel = 0;
    unsigned int sampleRate = 16000;
    unsigned int bufferFrames = 256;
    unsigned int numBuffers = 4;
    unsigned int numSamples = bufferFrames * numBuffers;
    unsigned int bitsPerSample = 16;
    unsigned int bytesPerSample = bitsPerSample / 8;
    unsigned int numChannels = 1;
    unsigned int fifoSize = numSamples * bytesPerSample;
    unsigned int preTriggerFifoSize = numSamples * bytesPerSample;
    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
    options.numberOfBuffers = numBuffers;
    options.priority = 99;
    options.streamName = "Audio Input Stream";
    audio.openStream(nullptr, &params, RTAUDIO_SINT16, sampleRate, &bufferFrames, &audioInputCallback, this, &options);
    

}

void SpeechIn::writeAudioDataToSocket(int16_t* buffer, size_t bufferSize)
{
}

int audioInputCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData)
{
    return 0;
}