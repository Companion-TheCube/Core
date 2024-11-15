#include "audioOutput.h"

/*
NOTES:
Because of how the HDMI output works, we will have to have a constant output and modulate the code to change the sound.

There will need to be a class (or something) that can take in sounds (ether buffers or file paths) and play them. 
This class will need to be able to play multiple sounds at once. 
*/

UserData AudioOutput::userData = { 0.0, 0.0, false };
bool AudioOutput::audioStarted = false;
std::unique_ptr<RtAudio> AudioOutput::dac = nullptr;

AudioOutput::AudioOutput()
{
    // TODO: load all the audio blobs from the DB.
    // TODO: load all the audio files from the filesystem.
    CubeLog::info("Initializing audio output.");
#ifdef __linux__
    RtAudio::Api api = RtAudio::RtAudio::LINUX_PULSE;
#elif _WIN32
    RtAudio::Api api = RtAudio::RtAudio::WINDOWS_DS;
#endif
    dac = std::make_unique<RtAudio>(api);
    // TODO: The RTAudio instance needs to be instantiated in the audioManager and passed to this class and to the speechIn class.
    std::vector<unsigned int> deviceIds = dac->getDeviceIds();
    std::vector<std::string> deviceNames = dac->getDeviceNames();
    if (deviceIds.size() < 1) {
        CubeLog::fatal("No audio devices found. Exiting.");
        exit(0);
    }
    CubeLog::info("Audio devices found: " + std::to_string(deviceIds.size()));
    for (auto device : deviceNames) {
        CubeLog::moreInfo("Device: " + device);
    }

    parameters = std::make_unique<RtAudio::StreamParameters>();
    CubeLog::info("Setting up audio stream with default audio device.");
    parameters->deviceId = dac->getDefaultOutputDevice();
    // find the device name for the audio device id
    std::string deviceName = dac->getDeviceInfo(parameters->deviceId).name;
    CubeLog::info("Using audio device: " + deviceName);
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
        if(!audioStarted)
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


HttpEndPointData_t AudioOutput::getHttpEndpointData(){
    HttpEndPointData_t data;
    data.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            this->start();
            CubeLog::info("Endpoint start called.");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Start audio called");
        },
        "start", {}, "Start the audio." });
    return data;
}

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

