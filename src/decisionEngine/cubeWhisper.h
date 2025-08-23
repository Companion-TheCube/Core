/*
 ██████╗██╗   ██╗██████╗ ███████╗██╗    ██╗██╗  ██╗██╗███████╗██████╗ ███████╗██████╗    ██╗  ██╗
██╔════╝██║   ██║██╔══██╗██╔════╝██║    ██║██║  ██║██║██╔════╝██╔══██╗██╔════╝██╔══██╗   ██║  ██║
██║     ██║   ██║██████╔╝█████╗  ██║ █╗ ██║███████║██║███████╗██████╔╝█████╗  ██████╔╝   ███████║
██║     ██║   ██║██╔══██╗██╔══╝  ██║███╗██║██╔══██║██║╚════██║██╔═══╝ ██╔══╝  ██╔══██╗   ██╔══██║
╚██████╗╚██████╔╝██████╔╝███████╗╚███╔███╔╝██║  ██║██║███████║██║     ███████╗██║  ██║██╗██║  ██║
 ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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

// CubeWhisper: local speech-to-text wrapper (whisper.cpp integration)
//
// Responsibilities
// - Initialize and manage whisper.cpp resources
// - Provide a simple static utility to transcribe short audio buffers
//
// Notes
// - Actual model loading and performance tuning TBD; see TODOs in .cpp
#ifndef CUBEWHISPER_H
#define CUBEWHISPER_H

#include <functional>
#include <httplib.h>
#include <iostream>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include "nlohmann/json.hpp"
#include "threadsafeQueue.h"
#include "utils.h"
#include "whisper.h"

class CubeWhisper {
public:
    CubeWhisper();
    static std::future<std::string> transcribe(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue);
    static std::string getPartialTranscription();

private:
    static std::mutex partialMutex;
    static std::string partialResult;
    static struct whisper_context* ctx;
    static std::jthread transcriberThread;
};

#endif // CUBEWHISPER_H
