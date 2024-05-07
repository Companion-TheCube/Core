#include <cstdlib>
#ifdef __linux__
#include <unistd.h>
#include <sys/wait.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <iostream>
#include "gui/renderer.h"
#include <SFML/Audio.hpp>
#include <cmath>
#include "logger/logger.h"

#ifdef _WIN32
#define M_PI 3.14159265358979323846
#endif

int main()
{
    #ifdef __linux__
    if(setenv("DISPLAY", ":0", 1) != 0)
    {
        std::cout<<"error"<<std::endl;
        return 1;
    }
    #endif
    
    
    auto logger = new CubeLog();

    const double sampleRate = 44100;
    const double frequency = 473;

    // Calculate the number of periods needed for a zero crossing at both ends
    double numPeriods = sampleRate / frequency;
    
    // Calculate the total number of samples
    int numSamples = sampleRate;// static_cast<int>(numPeriods * frequency);

    // Generate the samples
    std::vector<sf::Int16> sineWaveSamples;
    for (int i = 0; i < numSamples; ++i) {
        sf::Int16 sample = 10000 * std::sin(2 * M_PI * frequency * i / sampleRate);
        sineWaveSamples.push_back(sample);
    }

    sf::SoundBuffer sineWaveBuffer;
    sineWaveBuffer.loadFromSamples(&sineWaveSamples[0], sineWaveSamples.size(), 1, sampleRate);

    sf::Sound sineWaveSound;
    sineWaveSound.setBuffer(sineWaveBuffer);
    sineWaveSound.setLoop(true);
    sineWaveSound.play();

    auto renderer = new Renderer(logger);
    
    while(true)
    {
        #ifdef __linux__
        sleep(1);
        #endif
        #ifdef _WIN32
        Sleep(1000);
        #endif
    }
    return 0;
}