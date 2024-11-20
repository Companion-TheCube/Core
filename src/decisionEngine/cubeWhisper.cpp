#include "cubeWhisper.h"

Whisper::Whisper()
{
    // TODO: initialize whisper.cpp library and make sure it has the model loaded. 
    CubeLog::info("Whisper initialized");
}

std::string Whisper::transcribe(const std::string& audio)
{
    CubeLog::info("Transcribing audio");
    return "This is a test transcription";
}

/// Just testing that we can compile with whisper stuff
void whisperLoop(){
    struct whisper_context_params cparams = whisper_context_default_params();
    auto ctx = whisper_init_from_file_with_params("whisper-en-us-tdnn", cparams);   
}
