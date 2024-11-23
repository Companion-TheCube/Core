#ifndef SPEECHIN_H
#define SPEECHIN_H

#include "utils.h"
#include <logger.h>

#define SAMPLE_RATE 16000
#define NUM_CHANNELS 1
#define BITS_PER_SAMPLE 16
#define BYTES_PER_SAMPLE (BITS_PER_SAMPLE / 8)
// Audio data from RTAudio gets stored in a FIFO buffer to be read out by
// the speech recognition engine (openwakeword) and the decision engine.
// Expected data type is int16_t[]
#define ROUTER_FIFO_SIZE (512) // 32ms of audio at 16kHz 16bit mono
// The pre-trigger FIFO buffer is used to store audio data before the wake word
// is detected. Once the wake word is detected, the decision engine will read
// the audio data from the pre-trigger FIFO buffer to be analyzed for missed
// wake words.
// Expected data type is int16_t[]
#define PRE_TRIGGER_FIFO_SIZE (2 * 1000 * 16) // 2 seconds of audio at 16kHz 16bit mono


class SpeechIn
{
public:
    SpeechIn();
    ~SpeechIn();

    // Start the audio input thread
    void start();

    // Stop the audio input thread
    void stop();

    // Get the audio data from the FIFO buffer
    // Returns the number of bytes read
    size_t getAudioData(int16_t* buffer, size_t bufferSize);

    // Get the audio data from the pre-trigger FIFO buffer
    // Returns the number of bytes read
    size_t getPreTriggerAudioData(int16_t* buffer, size_t bufferSize);

    // Get the size of the audio data in the FIFO buffer
    size_t getAudioDataSize();

    // Get the size of the audio data in the pre-trigger FIFO buffer
    size_t getPreTriggerAudioDataSize();

    // Clear the audio data in the FIFO buffer
    void clearAudioData();

    // Clear the audio data in the pre-trigger FIFO buffer
    void clearPreTriggerAudioData();

    // Get the size of the FIFO buffer
    size_t getFifoSize();

    // Get the size of the pre-trigger FIFO buffer
    size_t getPreTriggerFifoSize();

    // Get the sample rate of the audio data
    unsigned int getSampleRate();

    // Get the number of channels in the audio data
    unsigned int getNumChannels();

    // Get the number of bits per sample in the audio data
    unsigned int getBitsPerSample();

    // Get the number of bytes per sample in the audio data
    unsigned int getBytesPerSample();

    // Get the number of samples in the audio data
    unsigned int getNumSamples();

    // Get the number of bytes in the audio data
    unsigned int getNumBytes();

private:
    // Audio data FIFO buffer
    std::vector<int16_t> audioData;
    // Pre-trigger audio data FIFO buffer
    std::vector<int16_t> preTriggerAudioData;
    // Mutex for the audio data FIFO buffer
    std::mutex audioDataMutex;
    // Mutex for the pre-trigger audio data FIFO buffer
    std::mutex preTriggerAudioDataMutex;
    // Condition variable for the audio data FIFO buffer
    std::condition_variable audioDataCV;
    // Condition variable for the pre-trigger audio data FIFO buffer
    std::condition_variable preTriggerAudioDataCV;
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
    void audioInputThread();

    // Write audio data from the FIFO to the unix socket
    void writeAudioDataToSocket(int16_t* buffer, size_t bufferSize);


};
#endif // SPEECHIN_H
