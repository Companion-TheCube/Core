/*
 █████╗ ██╗   ██╗██████╗ ██╗ ██████╗  ██████╗ ██╗   ██╗████████╗██████╗ ██╗   ██╗████████╗ ██████╗██████╗ ██████╗ 
██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗██╔═══██╗██║   ██║╚══██╔══╝██╔══██╗██║   ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗
███████║██║   ██║██║  ██║██║██║   ██║██║   ██║██║   ██║   ██║   ██████╔╝██║   ██║   ██║   ██║     ██████╔╝██████╔╝
██╔══██║██║   ██║██║  ██║██║██║   ██║██║   ██║██║   ██║   ██║   ██╔═══╝ ██║   ██║   ██║   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝╚██████╔╝╚██████╔╝   ██║   ██║     ╚██████╔╝   ██║██╗╚██████╗██║     ██║     
╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝  ╚═════╝  ╚═════╝    ╚═╝   ╚═╝      ╚═════╝    ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#ifndef LOGGER_H
#include <logger.h>
#endif
#include "audioOutput.h"
#include "../decisionEngine/remoteServer.h"
#include "httplib.h"

#include <SFML/Audio.hpp>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

/*
NOTES:
Because of how the HDMI output works, we will have to have a constant output and modulate the code to change the sound.

There will need to be a class (or something) that can take in sounds (ether buffers or file paths) and play them.
This class will need to be able to play multiple sounds at once.
*/

// TODO: Local TTS using Piper

UserData AudioOutput::userData = { 0.0, 0.0, false };
bool AudioOutput::audioStarted = false;
std::unique_ptr<RtAudio> AudioOutput::dac = nullptr;

namespace {

struct PlaybackState {
    std::vector<std::shared_ptr<sf::SoundBuffer>> buffers;
    std::vector<std::shared_ptr<sf::Sound>> sounds;
    std::mutex mutex;
};

PlaybackState& playbackState()
{
    static PlaybackState state;
    return state;
}

void cleanupFinishedSounds()
{
    auto& state = playbackState();
    std::lock_guard<std::mutex> lock(state.mutex);

    std::vector<std::shared_ptr<sf::SoundBuffer>> liveBuffers;
    std::vector<std::shared_ptr<sf::Sound>> liveSounds;
    liveBuffers.reserve(state.buffers.size());
    liveSounds.reserve(state.sounds.size());

    for (size_t i = 0; i < state.sounds.size(); ++i) {
        const auto& sound = state.sounds[i];
        if (sound && sound->getStatus() == sf::SoundSource::Playing) {
            liveSounds.push_back(sound);
            if (i < state.buffers.size()) {
                liveBuffers.push_back(state.buffers[i]);
            }
        }
    }

    state.buffers = std::move(liveBuffers);
    state.sounds = std::move(liveSounds);
}

bool startPlayback(const std::shared_ptr<sf::SoundBuffer>& buffer, float volume)
{
    if (!buffer) {
        return false;
    }

    cleanupFinishedSounds();

    auto sound = std::make_shared<sf::Sound>();
    sound->setBuffer(*buffer);
    sound->setVolume(std::clamp(volume, 0.0f, 100.0f));
    sound->play();

    auto& state = playbackState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.buffers.push_back(buffer);
    state.sounds.push_back(sound);
    return true;
}

std::string trimTrailingSlash(std::string url)
{
    while (!url.empty() && url.size() > 1 && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

bool hasScheme(const std::string& value)
{
    return value.rfind("http://", 0) == 0
        || value.rfind("https://", 0) == 0
        || value.rfind("ws://", 0) == 0
        || value.rfind("wss://", 0) == 0;
}

std::string normalizeHttpBaseUrl(std::string value)
{
    if (value.empty()) {
        return value;
    }
    if (value.rfind("ws://", 0) == 0) {
        value.replace(0, 5, "http://");
    } else if (value.rfind("wss://", 0) == 0) {
        value.replace(0, 6, "https://");
    } else if (!hasScheme(value)) {
        value = "http://" + value;
    }
    return trimTrailingSlash(std::move(value));
}

void configureHttpAuth(
    httplib::Client& client,
    const std::string& bearerToken,
    const std::string& apiKey,
    const std::string& serialNumber)
{
    httplib::Headers headers = {
        { "Content-Type", "application/json" },
        { "Accept", "application/json" }
    };
    if (!apiKey.empty()) {
        headers.insert({ "X-API-Key", apiKey });
        headers.insert({ "ApiKey", apiKey });
    }
    client.set_default_headers(headers);
    if (!bearerToken.empty()) {
        client.set_bearer_token_auth(bearerToken.c_str());
    } else if (!serialNumber.empty()) {
        client.set_basic_auth(serialNumber.c_str(), TheCubeServer::serialNumberToPassword(serialNumber).c_str());
    }
}

std::filesystem::path resolveAssetPath(const std::filesystem::path& input)
{
    if (input.empty()) {
        return {};
    }
    if (input.is_absolute() && std::filesystem::exists(input)) {
        return input;
    }

    const std::vector<std::filesystem::path> candidates = {
        input,
        std::filesystem::current_path() / input,
        std::filesystem::current_path() / ".." / input,
        std::filesystem::current_path() / "build" / "bin" / input
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }

    return input;
}

bool looksLikeMockAudio(const std::vector<unsigned char>& bytes)
{
    constexpr char prefix[] = "MOCK_AUDIO:";
    if (bytes.size() < sizeof(prefix) - 1) {
        return false;
    }
    return std::memcmp(bytes.data(), prefix, sizeof(prefix) - 1) == 0;
}

} // namespace

AudioOutput::AudioOutput()
{
    // TODO: load all the audio blobs from the DB.
    // TODO: load all the audio files from the filesystem.
    CubeLog::info("Initializing audio output.");

    RtAudio::Api api = RtAudio::RtAudio::LINUX_PULSE;

    dac = std::make_unique<RtAudio>(api);
    // TODO: The RTAudio instance needs to be instantiated in the audioManager and passed to this class and to the speechIn class.
    std::vector<unsigned int> deviceIds = dac->getDeviceIds();
    std::vector<std::string> deviceNames = dac->getDeviceNames();

    if (deviceIds.size() < 1) {
        CubeLog::fatal("No audio devices found. Exiting.");
        exit(0);
    }
    CubeLog::info("Audio devices found: " + std::to_string(deviceIds.size()));
    for (size_t i = 0; i < deviceNames.size(); i++) {
        CubeLog::moreInfo("Device (" + std::to_string(deviceIds.at(i)) + "): " + deviceNames.at(i));
    }

    parameters = std::make_unique<RtAudio::StreamParameters>();
    CubeLog::info("Setting up audio stream with default audio device.");
    parameters->deviceId = dac->getDefaultOutputDevice();
    // find the device name for the audio device id
    std::string deviceName = dac->getDeviceInfo(parameters->deviceId).name;
    CubeLog::info("Using audio device: " + deviceName);
    RtAudioFormat format = dac->getDeviceInfo(parameters->deviceId).nativeFormats;
    std::string formatStr = "";
    if (format & RTAUDIO_SINT8)
        formatStr += "SINT8 ";
    if (format & RTAUDIO_SINT16)
        formatStr += "SINT16 ";
    if (format & RTAUDIO_SINT24)
        formatStr += "SINT24 ";
    if (format & RTAUDIO_SINT32)
        formatStr += "SINT32 ";
    if (format & RTAUDIO_FLOAT32)
        formatStr += "FLOAT32 ";
    if (format & RTAUDIO_FLOAT64)
        formatStr += "FLOAT64 ";
    CubeLog::info("Audio device format: " + formatStr);
    parameters->nChannels = 2;
    parameters->firstChannel = 0;
    userData.data[0] = 0;
    userData.data[1] = 0;
    userData.soundOn = false;

    if (dac->openStream(parameters.get(), NULL, RTAUDIO_FLOAT64, 48000, &bufferFrames, &saw, (void*)&userData)) {
        CubeLog::fatal(dac->getErrorText() + " Exiting.");
        exit(1); // problem with device settings
        // TODO: Need to have a way to recover from this error.
        // TODO: Any call to exit should use an enum value to indicate the reason for the exit.
    }
}

AudioOutput::~AudioOutput()
{
    stop();
}

void AudioOutput::start()
{
    if (dac->startStream()) {
        std::cout << dac->getErrorText() << std::endl;
        CubeLog::error(dac->getErrorText());
        return;
    }
    audioStarted = true;
    CubeLog::info("Audio stream started.");
}

void AudioOutput::stop()
{
    if (dac->isStreamRunning()) {
        dac->stopStream();
        CubeLog::info("Audio stream stopped.");
    }
    audioStarted = false;
}

void AudioOutput::toggleSound()
{
    userData.soundOn = !userData.soundOn;
    if (userData.soundOn) {
        if (!audioStarted)
            AudioOutput::start();
        CubeLog::info("Sound on.");
    } else {
        CubeLog::info("Sound off.");
    }
}

void AudioOutput::setSound(bool soundOn)
{
    userData.soundOn = soundOn;
    if (userData.soundOn) {
        CubeLog::info("Sound on.");
    } else {
        CubeLog::info("Sound off.");
    }
}

bool AudioOutput::playFileAsync(const std::filesystem::path& filePath, float volume)
{
    const auto resolvedPath = resolveAssetPath(filePath);
    std::thread([resolvedPath, volume]() {
        try {
            auto buffer = std::make_shared<sf::SoundBuffer>();
            if (!buffer->loadFromFile(resolvedPath.string())) {
                CubeLog::error("AudioOutput: failed to load audio file: " + resolvedPath.string());
                return;
            }
            startPlayback(buffer, volume);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("AudioOutput: playFileAsync exception: ") + e.what());
        } catch (...) {
            CubeLog::error("AudioOutput: playFileAsync unknown exception");
        }
    }).detach();
    return true;
}

bool AudioOutput::playPcm16MonoAsync(const std::vector<int16_t>& samples, unsigned int sampleRateHz, float volume)
{
    if (samples.empty() || sampleRateHz == 0) {
        return false;
    }

    std::thread([samples, sampleRateHz, volume]() {
        try {
            auto buffer = std::make_shared<sf::SoundBuffer>();
            if (!buffer->loadFromSamples(samples.data(), samples.size(), 1, sampleRateHz)) {
                CubeLog::error("AudioOutput: failed to load PCM samples into sound buffer");
                return;
            }
            startPlayback(buffer, volume);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("AudioOutput: playPcm16MonoAsync exception: ") + e.what());
        } catch (...) {
            CubeLog::error("AudioOutput: playPcm16MonoAsync unknown exception");
        }
    }).detach();
    return true;
}

bool AudioOutput::speakTextAsync(const std::string& text, float volume)
{
    if (text.empty()) {
        return false;
    }

    std::thread([text, volume]() {
        try {
            const std::string configuredBase = Config::get(
                "REMOTE_SERVER_BASE_URL",
                Config::get("REMOTE_TRANSCRIPTION_BASE_URL", "https://api.4thecube.com"));
            const std::string baseUrl = normalizeHttpBaseUrl(configuredBase);
            if (baseUrl.empty()) {
                CubeLog::error("AudioOutput: no remote server base URL configured for speech synthesis");
                return;
            }

            httplib::Client client(baseUrl);
            client.set_read_timeout(30, 0);
            client.set_write_timeout(30, 0);
            client.set_connection_timeout(5, 0);

            const auto bearerToken = Config::get("REMOTE_SERVER_BEARER_TOKEN", Config::get("REMOTE_AUTH_KEY", ""));
            const auto apiKey = Config::get(
                "REMOTE_SERVER_API_KEY",
                Config::get("REMOTE_TRANSCRIPTION_API_KEY", Config::get("REMOTE_API_KEY", "")));
            const auto serialNumber = Config::get("REMOTE_SERVER_SERIAL", Config::get("DEVICE_SERIAL_NUMBER", ""));
            configureHttpAuth(client, bearerToken, apiKey, serialNumber);

            const nlohmann::json payload = {
                { "text", text },
                { "responseMode", "json" },
                { "outputFormat", "pcm" }
            };

            auto res = client.Post("/API/audio/synthesize", payload.dump(), "application/json");
            if (!res) {
                CubeLog::error("AudioOutput: speech synthesis request failed");
                return;
            }
            if (res->status != 200) {
                CubeLog::error("AudioOutput: speech synthesis returned status " + std::to_string(res->status));
                return;
            }

            auto json = nlohmann::json::parse(res->body);
            if (!json.contains("audioData") || !json["audioData"].is_string()) {
                CubeLog::error("AudioOutput: synthesis response did not include audioData");
                return;
            }

            const auto bytes = base64_decode_cube(json["audioData"].get<std::string>());
            if (bytes.empty()) {
                CubeLog::warning("AudioOutput: synthesis response returned empty audio");
                return;
            }
            if (looksLikeMockAudio(bytes)) {
                CubeLog::info("AudioOutput: mock synthesis audio received; skipping playback");
                return;
            }
            if ((bytes.size() % sizeof(int16_t)) != 0) {
                CubeLog::error("AudioOutput: synthesis audio was not aligned to 16-bit PCM");
                return;
            }

            unsigned int sampleRateHz = 16000;
            try {
                const auto configured = Config::get("REMOTE_TTS_PCM_SAMPLE_RATE_HZ", "16000");
                sampleRateHz = static_cast<unsigned int>(std::stoul(configured));
            } catch (...) {
                sampleRateHz = 16000;
            }

            std::vector<int16_t> samples(bytes.size() / sizeof(int16_t));
            std::memcpy(samples.data(), bytes.data(), bytes.size());
            if (!playPcm16MonoAsync(samples, sampleRateHz, volume)) {
                CubeLog::error("AudioOutput: failed to enqueue synthesized speech playback");
            }
        } catch (const std::exception& e) {
            CubeLog::error(std::string("AudioOutput: speakTextAsync exception: ") + e.what());
        } catch (...) {
            CubeLog::error("AudioOutput: speakTextAsync unknown exception");
        }
    }).detach();

    return true;
}

// TODO: create a function that the output can use to stream data from a ThreadSafeQueue<> to the audio output.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Two-channel sawtooth wave generator.
int saw(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData)
{
    unsigned int i, j;
    double* buffer = (double*)outputBuffer;
    UserData* data = (UserData*)userData;

    if (status)
        CubeLog::error("Stream underflow detected!");

    // Write interleaved audio data.
    for (i = 0; i < nBufferFrames; i++) {
        for (j = 0; j < 2; j++) {
            if (!data->soundOn)
                *buffer++ = 0.0;
            else
                *buffer++ = data->data[j];

            data->data[j] += 0.005 * (j + 1 + (j * 0.1));
            if (data->data[j] >= 1.0)
                data->data[j] -= 2.0;
        }
    }
    return 0;
}
