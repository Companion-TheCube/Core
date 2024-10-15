#include "audioManager.h"

AudioManager::AudioManager()
{
    audioOutput = new AudioOutput();
}

AudioManager::~AudioManager()
{
    delete audioOutput;
    stop();
}

void AudioManager::start()
{
    audioOutput->start();
}

void AudioManager::stop()
{
    audioOutput->stop();
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
            for (auto param : req.params) {
                if (param.first == "soundOn") {
                    this->setSound(param.second == "true");
                    p = param.second;
                }
            }
            CubeLog::info("Endpoint set sound called with param: " + p);
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Set sound called with param: " + p);
        },
        "setSound", { "soundOn" }, "Set sound to boolean state. \"true\" is on." });
    return data;
}

// std::vector<std::pair<std::string, std::vector<std::string>>> AudioManager::getHttpEndpointNamesAndParams()
// {
//     return {
//         { "start", {} },
//         { "stop", {} },
//         { "toggleSound", {} },
//         { "setSound", { "soundOn" } }
//     };
// }