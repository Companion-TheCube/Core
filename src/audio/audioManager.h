#pragma once
#include "../api/api.h"
#include "../threadsafeQueue.h"
#include "audioOutput.h"
#include "speechIn.h"
#include <utils.h>

class AudioManager : public AutoRegisterAPI<AudioManager> {
public:
    AudioManager();
    ~AudioManager();
    void start();
    void stop();
    void toggleSound();
    void setSound(bool soundOn);
    static std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioInQueue;

    // API Interface
    constexpr std::string getInterfaceName() const override { return "AudioManager"; }
    HttpEndPointData_t getHttpEndpointData() override;

private:
    std::unique_ptr<AudioOutput> audioOutput;
    std::shared_ptr<SpeechIn> speechIn;
};