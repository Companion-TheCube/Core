#include "cubeWhisper.h"

CubeWhisper::CubeWhisper()
{
    // TODO: initialize whisper.cpp library and make sure it has the model loaded. 
    CubeLog::info("CubeWhisper initialized");
}

std::string CubeWhisper::transcribe(const std::string& audio)
{
    CubeLog::info("Transcribing audio");
    return "This is a test transcription";
}
