#include <iostream>
#include <cmath>
#include "portaudio.h"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 64

// Function prototypes
static int sineWave(const void* inputBuffer, void* outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void* userData);

static int sineSquareWave(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

// Callback function to generate audio data - Default to sine wave
static int (*currentCallback)(const void*, void*, unsigned long,
                              const PaStreamCallbackTimeInfo*,
                              PaStreamCallbackFlags, void*) = sineSquareWave;

bool keyPressed() {
    return std::cin.rdbuf()->in_avail() != 0;
}

int main() {
    PaError err;
    PaStream* stream;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Open an audio stream
    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE,
                               FRAMES_PER_BUFFER, currentCallback, NULL);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::cout << "Press 's' to switch between sine wave and sine-square wave, and Enter to quit..." << std::endl;

    char input;
    char inputLine[256];
    do {
        std::cin.getline(inputLine, 256);
        input = inputLine[0];
        if (input == 's') {
            // Switch between sine wave and sine-square wave
            currentCallback = (currentCallback == sineWave) ? sineSquareWave : sineWave;
            // Reopen the stream with the new callback function
            err = Pa_CloseStream(stream);
            if (err != paNoError) {
                std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
            }
            err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE,
                                       FRAMES_PER_BUFFER, currentCallback, NULL);
            if (err != paNoError) {
                std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
                Pa_Terminate();
                return 1;
            }
            err = Pa_StartStream(stream);
            if (err != paNoError) {
                std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
                Pa_CloseStream(stream);
                Pa_Terminate();
                return 1;
            }
        }
    } while (input != '\n' && input != EOF);

    // Stop and close the audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    // Terminate PortAudio
    Pa_Terminate();

    return 0;
}

// Sine wave callback function
static int sineWave(const void* inputBuffer, void* outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void* userData) {
    float* out = (float*)outputBuffer;
    static float phase = 0.0f;
    static float deltaPhase = 440.0f / SAMPLE_RATE;

    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        // Generate sine wave
        float sample = 0.5f * std::sin(2.0f * 3.14159f * phase);

        // Output the sample to left and right channels
        *out++ = sample; // Left channel
        *out++ = sample; // Right channel

        // Update phase
        phase += deltaPhase;
        if (phase >= 1.0f) {
            phase -= 1.0f;
        }
    }

    return paContinue;
}

// Sine-square wave callback function
static int sineSquareWave(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    float* out = (float*)outputBuffer;
    static float sinePhase = 0.0f;
    static float squarePhase = 0.0f;
    static float deltaPhaseSine = 440.0f / SAMPLE_RATE;
    static float deltaPhaseSquare = 440.0f / SAMPLE_RATE;
    const float sineAmplitude = 0.5f;
    const float squareAmplitude = 0.5f;

    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        // Generate sine wave
        float sineSample = sineAmplitude * std::sin(2.0f * 3.14159f * sinePhase);
        
        // Generate square wave
        float squareSample = squarePhase < 0.5f ? -squareAmplitude : squareAmplitude;

        // Combine sine and square waves
        float combinedSample = sineSample + squareSample;

        // Output the combined sample to left and right channels
        *out++ = combinedSample; // Left channel
        *out++ = combinedSample; // Right channel

        // Update phases
        sinePhase += deltaPhaseSine;
        if (sinePhase >= 1.0f) {
            sinePhase -= 1.0f;
        }

        squarePhase += deltaPhaseSquare;
        if (squarePhase >= 1.0f) {
            squarePhase -= 1.0f;
        }
    }

    return paContinue;
}