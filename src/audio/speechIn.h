/*
███████╗██████╗ ███████╗███████╗ ██████╗██╗  ██╗██╗███╗   ██╗   ██╗  ██╗
██╔════╝██╔══██╗██╔════╝██╔════╝██╔════╝██║  ██║██║████╗  ██║   ██║  ██║
███████╗██████╔╝█████╗  █████╗  ██║     ███████║██║██╔██╗ ██║   ███████║
╚════██║██╔═══╝ ██╔══╝  ██╔══╝  ██║     ██╔══██║██║██║╚██╗██║   ██╔══██║
███████║██║     ███████╗███████╗╚██████╗██║  ██║██║██║ ╚████║██╗██║  ██║
╚══════╝╚═╝     ╚══════╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝╚═╝╚═╝  ╚═╝
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

#pragma once
#ifndef SPEECHIN_H
#define SPEECHIN_H
#include "RtAudio.h"
#include "utils.h"
#ifndef LOGGER_H
#include <logger.h>
#endif
#include "base64_rfc4648.hpp"
#include "threadsafeQueue.h"
// include time library for time point
#include <chrono>

#define SAMPLE_RATE 16000
#define NUM_CHANNELS 2
#define BITS_PER_SAMPLE 16
#define BYTES_PER_SAMPLE (BITS_PER_SAMPLE / 8)
// Audio data from RTAudio gets stored in a FIFO buffer to be read out by
// the speech recognition engine (openwakeword) and the decision engine.
// Expected data type is int16_t[]
#define ROUTER_FIFO_SIZE (1280) // 32ms of audio at 16kHz 16bit mono
// The pre-trigger FIFO buffer is used to store audio data before the wake word
// is detected. Once the wake word is detected, the decision engine will read
// the audio data from the pre-trigger FIFO buffer to be analyzed for missed
// wake words.
// Expected data type is int16_t[]
#define PRE_TRIGGER_FIFO_SIZE (5 * 1000 * 16) // 5 seconds of audio at 16kHz 16bit mono
#define SILENCE_TIME (30 * SAMPLE_RATE) // 0.7 seconds of averaged audio data
#define SILENCE_TIMEOUT (0.7 * SAMPLE_RATE) // 0.5 seconds of silence
#define WAKEWORD_RETRIGGER_TIME (5 * 1000) // 5 seconds

class SpeechIn {
public:
    SpeechIn() = default;
    ~SpeechIn();

    // Start the audio input thread
    void start();

    // Stop the audio input thread
    void stop();

    // Audio data FIFO buffer
    static std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue;
    // Pre-trigger audio data FIFO buffer. Maintains 5 seconds of audio data.
    static std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> preTriggerAudioData;

    // Subscribe to wake word detection events
    static void subscribeToWakeWordDetection(std::function<void()> callback)
    {
        // TODO: This function should return a handle that can be used to unsubscribe.
        // TODO: make sure this works
        // TODO: make sure this is thread safe
        SpeechIn::wakeWordDetectionCallbacks.push_back(callback);
    }
    // Unsubscribe from wake word detection events
    static void unsubscribeFromWakeWordDetection(std::function<void()> callback)
    {
        // TODO: This function should take a handle that was returned from subscribeToWakeWordDetection.
        // TODO: make sure this works
        // TODO: make sure this is thread safe
        // auto it = std::remove(SpeechIn::wakeWordDetectionCallbacks.begin(), SpeechIn::wakeWordDetectionCallbacks.end(), callback);
        // SpeechIn::wakeWordDetectionCallbacks.erase(it, SpeechIn::wakeWordDetectionCallbacks.end());
    }

private:
    // Flag to stop the audio input thread
    std::atomic<bool> stopFlag;
    // Sample rate of the audio data
    unsigned int sampleRate;
    // Number of channels in the audio data
    unsigned int numChannels;
    // Number of bits per sample in the audio data
    unsigned int bitsPerSample;
    // Number of bytes per sample in the audio data
    unsigned int bytesPerSample;
    // Number of samples in the audio data
    unsigned int numSamples;
    // Number of bytes in the audio data
    unsigned int numBytes;
    // Number of bytes in the FIFO buffer
    size_t fifoSize;
    // Number of bytes in the pre-trigger FIFO buffer
    size_t preTriggerFifoSize;
    // Audio input thread
    std::jthread audioInputThread;
    // Wake word detection callback vector
    static std::vector<std::function<void()>> wakeWordDetectionCallbacks;

    // Audio input thread
    void audioInputThreadFn(std::stop_token st);

    // Write audio data from the FIFO to the unix socket
    void writeAudioDataToSocket();
};

#endif // SPEECHIN_H
