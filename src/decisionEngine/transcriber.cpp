// Transcriber implementations: local (CubeWhisper) and remote (TheCubeServer).
// These are currently stubs. Buffer/stream methods should convert audio into
// text either by calling whisper.cpp locally or by streaming to the server.
#include "transcriber.h"


namespace DecisionEngine {
// TODO:
// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper class. Whisper class should be initialized and a reference to it saved for future use.
LocalTranscriber::LocalTranscriber()
{
    // TODO: construct CubeWhisper and prime any resources required for STT
}
LocalTranscriber::~LocalTranscriber()
{
}
std::string LocalTranscriber::transcribeBuffer(const uint16_t* audio, size_t length)
{
    // TODO: feed PCM buffer into CubeWhisper and return text
    return "";
}
std::string LocalTranscriber::transcribeStream(const uint16_t* audio, size_t bufSize)
{
    // TODO: consume from queue or stream interface; incremental transcription
    return "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RemoteTranscriber - class that interacts with the TheCube Server API.
RemoteTranscriber::RemoteTranscriber()
{
    // TODO: inject server API, set up session/state for streaming
}
RemoteTranscriber::~RemoteTranscriber()
{
}
std::string RemoteTranscriber::transcribeBuffer(const uint16_t* audio, size_t length)
{
    // TODO: upload buffer and poll for result
    return "";
}
std::string RemoteTranscriber::transcribeStream(const uint16_t* audio, size_t bufSize)
{
    // TODO: streaming upload with progress and partial results
    return "";
}

}
