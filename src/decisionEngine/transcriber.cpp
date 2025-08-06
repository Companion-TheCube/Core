#include "transcriber.h"


namespace DecisionEngine {
// TODO:
// Interface: Transcriber - class that converts audio to text
// LocalTranscriber - class that interacts with the whisper class. Whisper class should be initialized and a reference to it saved for future use.
LocalTranscriber::LocalTranscriber()
{
    // TODO: this whole stupid thing
}
LocalTranscriber::~LocalTranscriber()
{
}
std::string LocalTranscriber::transcribeBuffer(const uint16_t* audio, size_t length)
{
    return "";
}
std::string LocalTranscriber::transcribeStream(const uint16_t* audio, size_t bufSize)
{
    return "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RemoteTranscriber - class that interacts with the TheCube Server API.
RemoteTranscriber::RemoteTranscriber()
{
    // TODO: also all of this one too
}
RemoteTranscriber::~RemoteTranscriber()
{
}
std::string RemoteTranscriber::transcribeBuffer(const uint16_t* audio, size_t length)
{
    return "";
}
std::string RemoteTranscriber::transcribeStream(const uint16_t* audio, size_t bufSize)
{
    return "";
}

}