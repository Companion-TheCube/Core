#include "remoteServer.h"

// TheCubeServerAPI - class to interact with TheCube Server API. Will use API key stored in CubeDB. Key is stored encrypted and will be decrypted at load time.
// TODO: Implement this class

TheCubeServerAPI::TheCubeServerAPI(uint16_t* audioBuf)
{
    CubeLog::info("TheCubeServerAPI initialized");
}

TheCubeServerAPI::~TheCubeServerAPI()
{
    CubeLog::info("TheCubeServerAPI closing");
}

bool TheCubeServerAPI::initTranscribing()
{
    CubeLog::info("Initializing transcribing");
    return true;
}

bool TheCubeServerAPI::streamAudio()
{
    CubeLog::info("Streaming audio");
    return true;
}

bool TheCubeServerAPI::stopTranscribing()
{
    CubeLog::info("Stopping transcribing");
    return true;
}

