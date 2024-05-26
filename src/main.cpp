#include <cstdlib>
#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include "gui/gui.h"
#include <iostream>
// #include <SFML/Audio.hpp>
#include "logger/logger.h"
#include <RtAudio.h>
#include <cmath>
#include <functional>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "settings/loader.h"
#include "api/builder.h"


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
            if (lastValues[2] == 0)
                *buffer++ = 0.0;
            else
                *buffer++ = lastValues[j];

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
    // Set the display environment variable for linux
#ifdef __linux__
    if (setenv("DISPLAY", ":0", 1) != 0) {
        std::cout << "Error setting DISPLAY=:0 environment variable. Exiting." << std::endl;
        return 1;
    }
#endif
    GlobalSettings settings;
    auto logger = new CubeLog();
    auto settingsLoader = new SettingsLoader(logger, &settings);
    settingsLoader->loadSettings();
    logger->setVerbosity(settings.logVerbosity);
    logger->log("Logger initialized.", true);
    logger->log("Settings loaded.", true);

    
    /////////////////////////////////////////////////////////////////
    // RtAudio setup
    /////////////////////////////////////////////////////////////////
    RtAudio dac;
    std::vector<unsigned int> deviceIds = dac.getDeviceIds();
    std::vector<std::string> deviceNames = dac.getDeviceNames();
    if (deviceIds.size() < 1) {
        std::cout << "\nNo audio devices found! Exiting.\n";
        exit(0);
    }
    logger->log("Audio devices found: " + std::to_string(deviceIds.size()), true);
    for (auto device : deviceNames) {
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

    // auto gui = new GUI(logger);
    auto gui = std::make_shared<GUI>(logger);
    auto api = new API(logger);
    // auto api = std::make_shared<API>(logger);
    API_Builder api_builder(logger, api);
    api_builder.addInterface(gui);
    api_builder.start();
    bool running = true;
    logger->log("Entering main loop...", true);
    while (running) {
#ifdef __linux__
        sleep(1);
#endif
#ifdef _WIN32
        Sleep(1000);
#endif
        // get cin and check if it's "exit"
        std::string input;
        std::cin >> input;
        if (input == "exit" || input == "quit" || input == "q" || input == "e") {
            break;
        } else if (input == "sound") {
            if (data[2] == 0)
                data[2] = 1;
            else
                data[2] = 0;
        }
    }
    logger->log("Exiting main loop...", true);
    std::cout<<"Exiting..."<<std::endl;
    // sineWaveSound.stop();
    
    delete settingsLoader;
    logger->writeOutLogs();
    dac.stopStream();
    if (dac.isStreamOpen())
        dac.closeStream();
    // api->stop();
    delete api;
    delete logger;
    return 0;
}

