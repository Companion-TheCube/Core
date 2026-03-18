/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "audioCapture.h"
#include "audio/constants.h"
#ifndef LOGGER_H
#include <logger.h>
#endif

#include <memory>

namespace {
struct CaptureCtx {
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> q;
};

int rtCallback(void* /*outputBuffer*/, void* inputBuffer, unsigned int nBufferFrames,
               double /*streamTime*/, RtAudioStreamStatus /*status*/, void* userData)
{
    if (!userData || !inputBuffer || nBufferFrames == 0) return 0;
    auto* ctx = static_cast<CaptureCtx*>(userData);
    int16_t* buffer = static_cast<int16_t*>(inputBuffer);
    std::vector<int16_t> audio(buffer, buffer + nBufferFrames);
    ctx->q->push(std::move(audio));
    return 0;
}
}

AudioCapture::AudioCapture(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> targetQueue)
    : queue_(std::move(targetQueue)) {}

AudioCapture::~AudioCapture() { stop(); }

void AudioCapture::start()
{
    CubeLog::info("AudioCapture: start requested");
    stop_.store(false);
    thread_ = std::jthread([this](std::stop_token st) { this->run(st); });
}

void AudioCapture::stop()
{
    CubeLog::info("AudioCapture: stop requested");
    stop_.store(true);
    if (thread_.joinable()) thread_.request_stop();
}

void AudioCapture::run(std::stop_token st)
{
    try {
        RtAudio::Api api = RtAudio::RtAudio::LINUX_PULSE;
        std::unique_ptr<RtAudio> audio = std::make_unique<RtAudio>(api);
        if (audio->getDeviceCount() == 0) {
            CubeLog::error("AudioCapture: No audio devices found");
            return;
        }
        auto deviceId = audio->getDefaultInputDevice();
        RtAudio::StreamParameters params;
        params.deviceId = deviceId;
        params.nChannels = 1; // mono
        params.firstChannel = 0;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_NONINTERLEAVED;
        options.streamName = "AudioCapture";
        options.numberOfBuffers = 1;

        unsigned int bufferFrames = audio::ROUTER_FIFO_FRAMES; // capture block size
        CaptureCtx ctx{ queue_ };
        audio->openStream(nullptr, &params, RTAUDIO_SINT16, audio::SAMPLE_RATE, &bufferFrames, &rtCallback, &ctx, &options);
        auto info = audio->getDeviceInfo(deviceId);
        CubeLog::info("AudioCapture: opening device '" + info.name + "' id=" + std::to_string(deviceId) +
                      ", rate=" + std::to_string(audio::SAMPLE_RATE) +
                      ", frames=" + std::to_string(bufferFrames));
        audio->startStream();
        CubeLog::info("AudioCapture: stream started");

        while (!st.stop_requested() && !stop_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        audio->stopStream();
        CubeLog::info("AudioCapture: stream stopped");
        audio->closeStream();
    } catch (...) {
        CubeLog::error("AudioCapture: exception during run()");
    }
}
