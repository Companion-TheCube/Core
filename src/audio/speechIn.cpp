/*

In this file we will interact with the wake word detector to get wake word events.
We will also interact with the server API to:
    1. Translate speech to text (Whisper)
    2. Get the intent of the text (GPT4)
        a. Trigger the intent
        b. Get the response of the intent
    3. Provide feedback to the user in the form of on screen text and/or text to speech (Whisper)

The audioOutput.cpp file will need some sort static methods to allow classes in this file to interact with it.
Either that or we can have the relevant class from that file be instantiated as a shared pointer, and passed to the classes in this file.

OpenWakeWord will be used for wake word detection. It is a python script that the app manager will have to start. Audio will be sent to
that script via a named pipe. When a wake word is detected, the script will send a message back to the app manager via an http request.
We'll need to have an API endpoint that provides this functionality and triggers sending audio to the remote server.

When the wake word is detected, we'll need to tell the audioOutput class to play a sound to let the user know that the wake word was detected.

*/

#include "speechIn.h"

// Audio router
// This class will need a reference to the decision engine so that it can pass the audio to the it.
// There shall be a FIFO queue that will hold the audio data. When the decision engine is ready for audio,
// it will pull the audio from the queue. The audio router will have a method to add audio to the queue.
// The audio router will also have a method to start and stop the audio stream.
// The audio router will also have a method to get the number of audio samples in the queue.

// Wake word detector monitor (for making sure that the wake word detector is running)

std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> SpeechIn::audioQueue = std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>();
std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> SpeechIn::preTriggerAudioData = std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>();
int audioInputCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData);

SpeechIn::~SpeechIn()
{
    stop();
}

void SpeechIn::start()
{
    // Start the audio input thread that handles rtaudio and streams audio to openwakeword
    this->audioInputThread = std::jthread([this](std::stop_token st) {
        this->audioInputThreadFn(st);
    });
}

void SpeechIn::stop()
{
    this->audioInputThread.request_stop();
    this->audioInputThread.join();
}

size_t SpeechIn::getAudioDataSize()
{
    return 0;
}

size_t SpeechIn::getPreTriggerAudioDataSize()
{
    return 0;
}

void SpeechIn::clearAudioData()
{
}

void SpeechIn::clearPreTriggerAudioData()
{
}

size_t SpeechIn::getFifoSize()
{
    return 0;
}

size_t SpeechIn::getPreTriggerFifoSize()
{
    return 0;
}

unsigned int SpeechIn::getSampleRate()
{
    return sampleRate;
}

unsigned int SpeechIn::getNumChannels()
{
    return numChannels;
}

unsigned int SpeechIn::getBitsPerSample()
{
    return bitsPerSample;
}

unsigned int SpeechIn::getBytesPerSample()
{
    return bytesPerSample;
}

unsigned int SpeechIn::getNumSamples()
{
    return numSamples;
}

unsigned int SpeechIn::getNumBytes()
{
    return numSamples * bytesPerSample;
}

void SpeechIn::audioInputThreadFn(std::stop_token st)
{
    // set up rt audio
    RtAudio::Api api = RtAudio::RtAudio::LINUX_PULSE;
    std::unique_ptr audio = std::make_unique<RtAudio>(api);
    if (audio->getDeviceCount() == 0) {
        CubeLog::error("No audio devices found.");
        return;
    }

    std::vector<unsigned int> devicesList = audio->getDeviceIds();

    RtAudio::DeviceInfo info;
    for (const auto& i : devicesList) {
        info = audio->getDeviceInfo(i);
        if (info.inputChannels > 0) {
            CubeLog::info("Device " + std::to_string(i) + ": " + info.name);
            CubeLog::info("Input channels: " + std::to_string(info.inputChannels));
        }
    }

    auto soundInInd = audio->getDefaultInputDevice();

    RtAudio::StreamParameters params;
    params.deviceId = soundInInd;
    params.nChannels = 1;
    params.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_SINT16;
    options.streamName = "SpeechIn";
    options.numberOfBuffers = 1;

    unsigned int bufferFrames = 512;
    unsigned int sampleRate = 16000;

    try {
        audio->openStream(nullptr, &params, RTAUDIO_SINT16, sampleRate, &bufferFrames, &audioInputCallback, nullptr, &options);
        audio->startStream();
    } catch (RtAudioErrorType& e) {
        CubeLog::error("Error starting audio stream");
        return;
    }

    while (!st.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        this->writeAudioDataToSocket();
    }

    audio->stopStream();
    audio->closeStream();
}

void SpeechIn::writeAudioDataToSocket()
{
    using base64 = cppcodec::base64_rfc4648;
    auto data = this->audioQueue->pop();

    int sockfd;
#ifdef __linux__
    sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, "/home/andrew/Documents/projects/openwakeword/openww.sock");
#else
    sockaddr serverAddr;
    serverAddr.sa_family = AF_UNIX;
    strcpy(serverAddr.sa_data, "/home/andrew/Documents/projects/openwakeword/openww.sock");
#endif
    
    
    

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        CubeLog::error("Error opening socket");
        return;
    }

    if (connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        CubeLog::error("Error connecting to socket");
        return;
    }

    while (data) {
        // base64 encode the audio data
        std::string encodedData = base64::encode((const unsigned char*)data->data(), data->size() * sizeof(int16_t));

        // send the data to the server
        if (write(sockfd, encodedData.c_str(), encodedData.size()) < 0) {
            CubeLog::error("Error sending data to socket");
            return;
        }

        data = this->audioQueue->pop();
    }

    close(sockfd);
}

int audioInputCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData)
{
    int16_t* buffer = (int16_t*)inputBuffer;
    std::vector<int16_t> audioData(buffer, buffer + nBufferFrames);
    SpeechIn::audioQueue->push(audioData);
    return 0;
}