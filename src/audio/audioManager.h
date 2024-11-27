#include "../api/api.h"
#include "audioOutput.h"
#include <utils.h>

class AudioManager : public AutoRegisterAPI<AudioManager> {
public:
    AudioManager();
    ~AudioManager();
    void start();
    void stop();
    void toggleSound();
    void setSound(bool soundOn);
    constexpr std::string getInterfaceName() const override { return "AudioManager"; }
    HttpEndPointData_t getHttpEndpointData() override;

private:
    std::unique_ptr<AudioOutput> audioOutput;
};