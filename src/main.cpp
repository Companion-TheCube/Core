#include <cstdlib>
#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "gui/gui.h"
#include <iostream>
// #include <SFML/Audio.hpp>
#include "logger/logger.h"
#include <RtAudio.h>
#include <cmath>

#ifdef _WIN32
#define M_PI 3.14159265358979323846
#endif

// Two-channel sawtooth wave generator.
int saw(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData)
{
    unsigned int i, j;
    double* buffer = (double*)outputBuffer;
    double* lastValues = (double*)userData;

    if (status)
        std::cout << "Stream underflow detected!" << std::endl;

    // Write interleaved audio data.
    for (i = 0; i < nBufferFrames; i++) {
        for (j = 0; j < 2; j++) {
            if(lastValues[2]==0)*buffer++ = 0.0;
            else *buffer++ = lastValues[j];

            lastValues[j] += 0.005 * (j + 1 + (j * 0.1));
            if (lastValues[j] >= 1.0)
                lastValues[j] -= 2.0;
        }
    }

    return 0;
}

int main()
{
    std::cout << "Starting..." << std::endl;
#ifdef __linux__
    if (setenv("DISPLAY", ":0", 1) != 0) {
        std::cout << "Error setting DISPLAY=:0 environment variable. Exiting." << std::endl;
        return 1;
    }
#endif

    auto logger = new CubeLog();

    // const double sampleRate = 44100;
    // const double frequency = 473;

    // // Calculate the number of periods needed for a zero crossing at both ends
    // double numPeriods = sampleRate / frequency;

    // // Calculate the total number of samples
    // int numSamples = sampleRate;// static_cast<int>(numPeriods * frequency);

    // // Generate the samples
    // std::vector<sf::Int16> sineWaveSamples;
    // for (int i = 0; i < numSamples; ++i) {
    //     sf::Int16 sample = 10000 * std::sin(2 * M_PI * frequency * i / sampleRate);
    //     sineWaveSamples.push_back(sample);
    // }

    // sf::SoundBuffer sineWaveBuffer;
    // sineWaveBuffer.loadFromSamples(&sineWaveSamples[0], sineWaveSamples.size(), 1, sampleRate);

    // sf::Sound sineWaveSound;
    // sineWaveSound.setBuffer(sineWaveBuffer);
    // sineWaveSound.setLoop(true);
    // sineWaveSound.play();

    /////////////////////////////////////////////////////////////////
    RtAudio dac;
    std::vector<unsigned int> deviceIds = dac.getDeviceIds();
    std::vector<std::string> deviceNames = dac.getDeviceNames();
    if (deviceIds.size() < 1) {
        std::cout << "\nNo audio devices found! Exiting.\n";
        exit(0);
    }
    logger->log("Audio devices found: " + std::to_string(deviceIds.size()), true);
    for(auto device: deviceNames){
        logger->log("Device: " + device, true);
    }

    RtAudio::StreamParameters parameters;
    logger->log("Setting up audio stream with default audio device.", true);
    parameters.deviceId = dac.getDefaultOutputDevice();
    // find the device name for the audio device id
    std::string deviceName = dac.getDeviceInfo(parameters.deviceId).name;
    logger->log("Using audio device: " + deviceName, true);
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 256; // 256 sample frames
    double data[3] = { 0, 0, 0 };

    if (dac.openStream(&parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &saw, (void*)&data)) {
        logger->error(dac.getErrorText() + " Exiting.");
        exit(0); // problem with device settings
    }

    // Stream is open ... now start it.
    if (dac.startStream()) {
        std::cout << dac.getErrorText() << std::endl;
        logger->error(dac.getErrorText());
    }

    /////////////////////////////////////////////////////////////////

    auto gui = new GUI(logger);
    logger->log("Entering main loop...", true);
    while (true) {
#ifdef __linux__
        sleep(1);
#endif
#ifdef _WIN32
        Sleep(1000);
#endif
        // get cin and check if it's "exit"
        std::string input;
        std::cin >> input;
        if (input == "exit") {
            break;
        } else if (input == "sound") {
            if(data[2] == 0) data[2] = 1;
            else data[2] = 0;
        }
    }
    logger->log("Exiting main loop...", true);
    // std::cout<<"Exiting..."<<std::endl;
    logger->writeOutLogs();
    return 0;
}