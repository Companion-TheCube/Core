#include <utils.h>
#include "../api/api_i.h"
#include "audioOutput.h"

class AudioManager : public I_API_Interface {
public:
    AudioManager();
    ~AudioManager();
    void start();
    void stop();
    void toggleSound();
    void setSound(bool soundOn);
    std::string getInterfaceName() const override { return "AudioManager"; }
    HttpEndPointData_t getHttpEndpointData() override;
private:
    std::unique_ptr<AudioOutput> audioOutput;
};