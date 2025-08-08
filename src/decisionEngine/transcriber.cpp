/*
████████╗██████╗  █████╗ ███╗   ██╗███████╗ ██████╗██████╗ ██╗██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗ 
╚══██╔══╝██╔══██╗██╔══██╗████╗  ██║██╔════╝██╔════╝██╔══██╗██║██╔══██╗██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
   ██║   ██████╔╝███████║██╔██╗ ██║███████╗██║     ██████╔╝██║██████╔╝█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
   ██║   ██╔══██╗██╔══██║██║╚██╗██║╚════██║██║     ██╔══██╗██║██╔══██╗██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
   ██║   ██║  ██║██║  ██║██║ ╚████║███████║╚██████╗██║  ██║██║██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║     
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


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
