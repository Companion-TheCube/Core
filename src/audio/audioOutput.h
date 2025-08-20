/*
 █████╗ ██╗   ██╗██████╗ ██╗ ██████╗  ██████╗ ██╗   ██╗████████╗██████╗ ██╗   ██╗████████╗██╗  ██╗
██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗██╔═══██╗██║   ██║╚══██╔══╝██╔══██╗██║   ██║╚══██╔══╝██║  ██║
███████║██║   ██║██║  ██║██║██║   ██║██║   ██║██║   ██║   ██║   ██████╔╝██║   ██║   ██║   ███████║
██╔══██║██║   ██║██║  ██║██║██║   ██║██║   ██║██║   ██║   ██║   ██╔═══╝ ██║   ██║   ██║   ██╔══██║
██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝╚██████╔╝╚██████╔╝   ██║   ██║     ╚██████╔╝   ██║██╗██║  ██║
╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝  ╚═════╝  ╚═════╝    ╚═╝   ╚═╝      ╚═════╝    ╚═╝╚═╝╚═╝  ╚═╝
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

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

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
    static std::unique_ptr<RtAudio> dac;
    static UserData userData;
    std::unique_ptr<RtAudio::StreamParameters> parameters;
    unsigned int bufferFrames = 256; // 256 sample frames
};

int saw(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData);

#endif// AUDIOOUTPUT_H