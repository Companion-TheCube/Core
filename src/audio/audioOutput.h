#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#include <RtAudio.h>
#include <cmath>
#include <iostream>
#include <utils.h>

struct UserData {
    double data[2];
    bool soundOn;
};

class AudioOutput {
public:
    AudioOutput();
    ~AudioOutput();
    static void start();
    static void stop();
    static void toggleSound();
    static void setSound(bool soundOn);

private:
    static bool audioStarted;
    static RtAudio* dac;
    static UserData userData;
    RtAudio::StreamParameters* parameters;
    unsigned int bufferFrames = 256; // 256 sample frames
};

int saw(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData);