/*
 █████╗ ██╗   ██╗██████╗ ██╗ ██████╗  ██████╗ ██╗   ██╗████████╗██████╗ ██╗   ██╗████████╗ ██████╗██████╗ ██████╗ 
██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗██╔═══██╗██║   ██║╚══██╔══╝██╔══██╗██║   ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗
███████║██║   ██║██║  ██║██║██║   ██║██║   ██║██║   ██║   ██║   ██████╔╝██║   ██║   ██║   ██║     ██████╔╝██████╔╝
██╔══██║██║   ██║██║  ██║██║██║   ██║██║   ██║██║   ██║   ██║   ██╔═══╝ ██║   ██║   ██║   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝╚██████╔╝╚██████╔╝   ██║   ██║     ╚██████╔╝   ██║██╗╚██████╗██║     ██║     
╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝  ╚═════╝  ╚═════╝    ╚═╝   ╚═╝      ╚═════╝    ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#ifndef LOGGER_H
#include <logger.h>
#endif
#include "audioOutput.h"

/*
NOTES:
Because of how the HDMI output works, we will have to have a constant output and modulate the code to change the sound.

There will need to be a class (or something) that can take in sounds (ether buffers or file paths) and play them.
This class will need to be able to play multiple sounds at once.
*/

// TODO: Local TTS using Piper

UserData AudioOutput::userData = { 0.0, 0.0, false };
bool AudioOutput::audioStarted = false;
std::unique_ptr<RtAudio> AudioOutput::dac = nullptr;

AudioOutput::AudioOutput()
{
    // TODO: load all the audio blobs from the DB.
    // TODO: load all the audio files from the filesystem.
    CubeLog::info("Initializing audio output.");

    RtAudio::Api api = RtAudio::RtAudio::LINUX_PULSE;

    dac = std::make_unique<RtAudio>(api);
    // TODO: The RTAudio instance needs to be instantiated in the audioManager and passed to this class and to the speechIn class.
    std::vector<unsigned int> deviceIds = dac->getDeviceIds();
    std::vector<std::string> deviceNames = dac->getDeviceNames();

    if (deviceIds.size() < 1) {
        CubeLog::fatal("No audio devices found. Exiting.");
        exit(0);
    }
    CubeLog::info("Audio devices found: " + std::to_string(deviceIds.size()));
    for (size_t i = 0; i < deviceNames.size(); i++) {
        CubeLog::moreInfo("Device (" + std::to_string(deviceIds.at(i)) + "): " + deviceNames.at(i));
    }

    parameters = std::make_unique<RtAudio::StreamParameters>();
    CubeLog::info("Setting up audio stream with default audio device.");
    parameters->deviceId = dac->getDefaultOutputDevice();
    // find the device name for the audio device id
    std::string deviceName = dac->getDeviceInfo(parameters->deviceId).name;
    CubeLog::info("Using audio device: " + deviceName);
    RtAudioFormat format = dac->getDeviceInfo(parameters->deviceId).nativeFormats;
    std::string formatStr = "";
    if (format & RTAUDIO_SINT8)
        formatStr += "SINT8 ";
    if (format & RTAUDIO_SINT16)
        formatStr += "SINT16 ";
    if (format & RTAUDIO_SINT24)
        formatStr += "SINT24 ";
    if (format & RTAUDIO_SINT32)
        formatStr += "SINT32 ";
    if (format & RTAUDIO_FLOAT32)
        formatStr += "FLOAT32 ";
    if (format & RTAUDIO_FLOAT64)
        formatStr += "FLOAT64 ";
    CubeLog::info("Audio device format: " + formatStr);
    parameters->nChannels = 2;
    parameters->firstChannel = 0;
    userData.data[0] = 0;
    userData.data[1] = 0;
    userData.soundOn = false;

    if (dac->openStream(parameters.get(), NULL, RTAUDIO_FLOAT64, 48000, &bufferFrames, &saw, (void*)&userData)) {
        CubeLog::fatal(dac->getErrorText() + " Exiting.");
        exit(1); // problem with device settings
        // TODO: Need to have a way to recover from this error.
        // TODO: Any call to exit should use an enum value to indicate the reason for the exit.
    }
}

AudioOutput::~AudioOutput()
{
    stop();
}

void AudioOutput::start()
{
    if (dac->startStream()) {
        std::cout << dac->getErrorText() << std::endl;
        CubeLog::error(dac->getErrorText());
        return;
    }
    audioStarted = true;
    CubeLog::info("Audio stream started.");
}

void AudioOutput::stop()
{
    if (dac->isStreamRunning()) {
        dac->stopStream();
        CubeLog::info("Audio stream stopped.");
    }
    audioStarted = false;
}

void AudioOutput::toggleSound()
{
    userData.soundOn = !userData.soundOn;
    if (userData.soundOn) {
        if (!audioStarted)
            AudioOutput::start();
        CubeLog::info("Sound on.");
    } else {
        CubeLog::info("Sound off.");
    }
}

void AudioOutput::setSound(bool soundOn)
{
    userData.soundOn = soundOn;
    if (userData.soundOn) {
        CubeLog::info("Sound on.");
    } else {
        CubeLog::info("Sound off.");
    }
}

// TODO: create a function that the output can use to stream data from a ThreadSafeQueue<> to the audio output.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Two-channel sawtooth wave generator.
int saw(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData)
{
    unsigned int i, j;
    double* buffer = (double*)outputBuffer;
    UserData* data = (UserData*)userData;

    if (status)
        CubeLog::error("Stream underflow detected!");

    // Write interleaved audio data.
    for (i = 0; i < nBufferFrames; i++) {
        for (j = 0; j < 2; j++) {
            if (!data->soundOn)
                *buffer++ = 0.0;
            else
                *buffer++ = data->data[j];

            data->data[j] += 0.005 * (j + 1 + (j * 0.1));
            if (data->data[j] >= 1.0)
                data->data[j] -= 2.0;
        }
    }
    return 0;
}
