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
#include "threadsafeQueue.h"
// include time library for time point
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <functional>

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
    static size_t subscribeToWakeWordDetection(std::function<void()> callback)
    {
        SpeechIn::wakeWordDetectionCallbacks[handle] = callback;
        return handle++;
    }
    // Unsubscribe from wake word detection events
    static bool unsubscribeFromWakeWordDetection(size_t handle)
    {
        return SpeechIn::wakeWordDetectionCallbacks.erase(handle) > 0;
    }
    // Register/unregister queues to receive audio once the wake word is detected
    static size_t registerWakeAudioQueue(const std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>& queue)
    {
        std::lock_guard<std::mutex> lock(SpeechIn::registeredQueuesMutex);
        // Cleanup expired entries first
        for (auto it = SpeechIn::registeredWakeAudioQueues.begin(); it != SpeechIn::registeredWakeAudioQueues.end(); ) {
            if (it->second.expired()) it = SpeechIn::registeredWakeAudioQueues.erase(it);
            else ++it;
        }
        const size_t id = handle++;
        SpeechIn::registeredWakeAudioQueues.emplace(id, std::weak_ptr<ThreadSafeQueue<std::vector<int16_t>>>(queue));
        return id;
    }
    static bool unregisterWakeAudioQueue(size_t handleId)
    {
        std::lock_guard<std::mutex> lock(SpeechIn::registeredQueuesMutex);
        return SpeechIn::registeredWakeAudioQueues.erase(handleId) > 0;
    }
    // Register/unregister queues to receive pre-trigger audio (rolling buffer)
    static size_t registerPreTriggerAudioQueue(const std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>& queue)
    {
        std::lock_guard<std::mutex> lock(SpeechIn::registeredQueuesMutex);
        // Cleanup expired entries first
        for (auto it = SpeechIn::registeredPreTriggerAudioQueues.begin(); it != SpeechIn::registeredPreTriggerAudioQueues.end(); ) {
            if (it->second.expired()) it = SpeechIn::registeredPreTriggerAudioQueues.erase(it);
            else ++it;
        }
        const size_t id = handle++;
        SpeechIn::registeredPreTriggerAudioQueues.emplace(id, std::weak_ptr<ThreadSafeQueue<std::vector<int16_t>>>(queue));
        return id;
    }
    static bool unregisterPreTriggerAudioQueue(size_t handleId)
    {
        std::lock_guard<std::mutex> lock(SpeechIn::registeredQueuesMutex);
        return SpeechIn::registeredPreTriggerAudioQueues.erase(handleId) > 0;
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
    static std::unordered_map<unsigned int,std::function<void()>> wakeWordDetectionCallbacks;
    static std::atomic<unsigned int> handle;
    // Queues to receive audio on wake word detection
    static std::unordered_map<size_t, std::weak_ptr<ThreadSafeQueue<std::vector<int16_t>>>> registeredWakeAudioQueues;
    static std::mutex registeredQueuesMutex;
    // Queues to receive rolling pre-trigger audio
    static std::unordered_map<size_t, std::weak_ptr<ThreadSafeQueue<std::vector<int16_t>>>> registeredPreTriggerAudioQueues;

    // Audio input thread
    void audioInputThreadFn(std::stop_token st);

    // Write audio data from the FIFO to the unix socket
    void writeAudioDataToSocket();
};

#endif // SPEECHIN_H
