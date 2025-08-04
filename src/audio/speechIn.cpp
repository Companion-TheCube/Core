/*
███████╗██████╗ ███████╗███████╗ ██████╗██╗  ██╗██╗███╗   ██╗    ██████╗██████╗ ██████╗ 
██╔════╝██╔══██╗██╔════╝██╔════╝██╔════╝██║  ██║██║████╗  ██║   ██╔════╝██╔══██╗██╔══██╗
███████╗██████╔╝█████╗  █████╗  ██║     ███████║██║██╔██╗ ██║   ██║     ██████╔╝██████╔╝
╚════██║██╔═══╝ ██╔══╝  ██╔══╝  ██║     ██╔══██║██║██║╚██╗██║   ██║     ██╔═══╝ ██╔═══╝ 
███████║██║     ███████╗███████╗╚██████╗██║  ██║██║██║ ╚████║██╗╚██████╗██║     ██║     
╚══════╝╚═╝     ╚══════╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
std::string checkForControl(int conn_fd);
std::vector<std::function<void()>> SpeechIn::wakeWordDetectionCallbacks;

int16_t runningAvgSample = 0;
int64_t runningAvgCount = 0;

// Last time we detected the wake word
std::chrono::time_point<std::chrono::high_resolution_clock> lastDetectedTime = std::chrono::high_resolution_clock::now();

SpeechIn::~SpeechIn()
{
    stop();
}

void SpeechIn::start()
{
    // Start the audio input thread that handles rtaudio and streams audio to openwakeword
    stopFlag.store(false);
    this->audioInputThread = std::jthread([this](std::stop_token st) {
        this->audioInputThreadFn(st);
    });
}

void SpeechIn::stop()
{
    stopFlag.store(true);
    this->audioInputThread.request_stop();
    // this->audioInputThread.join();
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
    params.nChannels = NUM_CHANNELS;
    params.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_SINT16;
    options.streamName = "SpeechIn";
    options.numberOfBuffers = 1;

    unsigned int bufferFrames = ROUTER_FIFO_SIZE;

    try {
        audio->openStream(nullptr, &params, RTAUDIO_SINT16, SAMPLE_RATE, &bufferFrames, &audioInputCallback, nullptr, &options);
        audio->startStream();
    } catch (RtAudioErrorType& e) {
        CubeLog::error("Error starting audio stream");
        return;
    }

    while (!st.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Wait one second before trying to connect to the socket.
        this->writeAudioDataToSocket();
    }

    audio->stopStream();
    audio->closeStream();
}

void SpeechIn::writeAudioDataToSocket()
{
    // using base64 = cppcodec::base64_rfc4648;
    auto dataOpt = this->audioQueue->pop();
    // openwakeword creates a unix socket at /tmp/openww. We send the audio data to this socket.
    int sockfd;
    struct sockaddr_un serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    std::strncpy(serverAddr.sun_path, "/tmp/openww", sizeof(serverAddr.sun_path) - 1);

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        CubeLog::error("Error opening socket");
        return;
    }

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        CubeLog::error("Error connecting to socket");
        close(sockfd);
        return;
    }

    while (dataOpt && !stopFlag.load()) {
        auto& data = *dataOpt;
        // Add the audio data to the 5 second buffer
        for (size_t i = 0; i < data.size(); i++) {
            preTriggerAudioData->push(data);
        }
        // truncate the audio data to 5 seconds
        while (preTriggerAudioData->size() > PRE_TRIGGER_FIFO_SIZE) {
            preTriggerAudioData->pop();
        }

        // send the data to the server
        if (write(sockfd, data.data(), data.size() * sizeof(int16_t)) < 0) {
            CubeLog::error("Error sending data to socket");
            close(sockfd);
            return;
        }

        dataOpt = this->audioQueue->pop();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto result = checkForControl(sockfd);
        if (result.find("DETECTED____") != std::string::npos) {
            // get current time
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDetectedTime).count();
            lastDetectedTime = now;
            CubeLog::info("Wake word detected. Duration since last detection: " + std::to_string(duration) + "ms");
            if (duration > WAKEWORD_RETRIGGER_TIME) {
                // call all the callbacks
                // TODO: this needs to happen in another thread so that we don't block the audio input thread
                for (auto& callback : this->wakeWordDetectionCallbacks) {
                    callback();
                }
            }
        }
    }
    if (stopFlag.load()) {
        close(sockfd);
        return;
    }
    CubeLog::error("No data in queue");
    close(sockfd);
}



int audioInputCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData)
{
    int16_t* buffer = (int16_t*)inputBuffer;
    std::vector<int16_t> audioData(buffer, buffer + nBufferFrames);
    SpeechIn::audioQueue->push(audioData);
    return 0;
}

std::string checkForControl(int conn_fd)
{
    // simple line-buffered read:
    constexpr size_t BUF_SIZE = 128;
    char buf[BUF_SIZE];
    ssize_t got = recv(conn_fd, buf, BUF_SIZE - 1, 0);
    if (got > 0) {
        buf[got] = '\0';
        std::string line(buf);
        // strip newline if present
        if (!line.empty() && line.back() == '\n')
            line.pop_back();
        // CubeLog::info("Received: " + line);
        return line;
    } else if (got == 0) {
        std::cerr << "Python closed the socket\n";
    } else {
        std::cerr << "recv() error: " << strerror(errno) << "\n";
    }
    return "";
}

// TODO: implement silence detection to determine when the user is done speaking.
