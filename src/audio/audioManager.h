#include "../api/api_i.h"
#include "audioOutput.h"
#include <utils.h>

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
    // std::vector<std::pair<std::string,std::vector<std::string>>> getHttpEndpointNamesAndParams() override;
private:
    AudioOutput* audioOutput;
};