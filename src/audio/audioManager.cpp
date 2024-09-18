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
            return "Start audio called";
        } });
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->stop();
            CubeLog::info("Endpoint stop called");
            return "Stop audio called";
        } });
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->toggleSound();
            CubeLog::info("Endpoint toggle sound called");
            return "Toggle sound called";
        } });
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
            return "Set sound called";
        } });
    return data;
}

std::vector<std::pair<std::string,std::vector<std::string>>> AudioManager::getHttpEndpointNamesAndParams()
{
    return {
        {"start", {}},
        {"stop", {}},
        {"toggleSound", {}},
        {"setSound", {"soundOn"}}
    };
}