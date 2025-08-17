/*
 █████╗ ██╗   ██╗██████╗ ██╗ ██████╗ ███╗   ███╗ █████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗ 
██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
███████║██║   ██║██║  ██║██║██║   ██║██╔████╔██║███████║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔══██║██║   ██║██║  ██║██║██║   ██║██║╚██╔╝██║██╔══██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝██║ ╚═╝ ██║██║  ██║██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║     
╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
#include "audioManager.h"

std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> AudioManager::audioInQueue = nullptr;



// TODO: NOTE: Audio input needs to be configured for RTAUDIO_INT16 or all the threadsafeQueue 
// stuff needs to be changed to handle the different data types.


// TODO: this whole thing, basically
AudioManager::AudioManager()
{
    audioOutput = std::make_unique<AudioOutput>();
    audioInQueue = std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>();
    speechIn = std::make_shared<SpeechIn>();
    speechIn->start();
    SpeechIn::subscribeToWakeWordDetection([this]() {
        CubeLog::info("Wake word detected.");
    });
}

AudioManager::~AudioManager()
{
    stop();
}

void AudioManager::start()
{
    audioOutput->start();
}

void AudioManager::stop()
{
    audioOutput->stop();
    speechIn->stop();
}

void AudioManager::toggleSound()
{
    AudioOutput::toggleSound();
}

void AudioManager::setSound(bool soundOn)
{
    AudioOutput::setSound(soundOn);
}

HttpEndPointData_t AudioManager::getHttpEndpointData()
{
    // TODO: Additional endpoints will be defined in the audioOutput.cpp file and speechIn.cpp file so we
    //  need to pull those in here and add them to the data vector.
    HttpEndPointData_t data;
    data.push_back({
        PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req,
        httplib::Response& res) {
            this->start();
            CubeLog::info("Endpoint start called.");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        "Start the audio."
    })
    data.push_back({
        PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req,
        httplib::Response& res) {
            this->stop();
            CubeLog::info("Endpoint stop called");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        "Stop the audio."
    })
    data.push_back({
        PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req,
        httplib::Response& res) {
            this->toggleSound();
            CubeLog::info("Endpoint toggle sound called");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        "Toggle the sound."
    })
    data.push_back({
        PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req,
        httplib::Response& res) {
            std::string p = "no param";
            for (auto& [paramName, paramValue] : req.params) {
                if (paramName == "soundOn") {
                    this->setSound(paramValue == "true");
                    p = paramValue;
                }
            }
            CubeLog::info("Endpoint set sound called with param: " + p);
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        nlohmann::json({ { "type", "object" }, { "properties", { { "soundOn", { { "type", "boolean" } } } } }, { "required", nlohmann::json::array({ "soundOn" }) } }),
        "Set sound to boolean state. \"true\" is on."
    })
    return data;
}
