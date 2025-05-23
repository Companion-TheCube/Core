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
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->start();
            CubeLog::info("Endpoint start called.");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Start audio called");
        },
        "start", {}, "Start the audio." });
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->stop();
            CubeLog::info("Endpoint stop called");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Stop audio called");
        },
        "stop", {}, "Stop the audio." });
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->toggleSound();
            CubeLog::info("Endpoint toggle sound called");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Toggle sound called");
        },
        "toggleSound", {}, "Toggle the sound." });
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            std::string p = "no param";
            for (auto& [paramName, paramValue] : req.params) {
                if (paramName == "soundOn") {
                    this->setSound(paramValue == "true");
                    p = paramValue;
                }
            }
            CubeLog::info("Endpoint set sound called with param: " + p);
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Set sound called with param: " + p);
        },
        "setSound", { "soundOn" }, "Set sound to boolean state. \"true\" is on." });
    return data;
}