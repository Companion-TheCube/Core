/*
 █████╗ ██╗   ██╗██████╗ ██╗ ██████╗ ███╗   ███╗ █████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗    ██╗  ██╗
██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗   ██║  ██║
███████║██║   ██║██║  ██║██║██║   ██║██╔████╔██║███████║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝   ███████║
██╔══██║██║   ██║██║  ██║██║██║   ██║██║╚██╔╝██║██╔══██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗   ██╔══██║
██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝██║ ╚═╝ ██║██║  ██║██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║██╗██║  ██║
╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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

#pragma once
#include "../api/api.h"
#include "../threadsafeQueue.h"
#include "audioOutput.h"
#include "speechIn.h"
#include <utils.h>

class AudioManager : public AutoRegisterAPI<AudioManager> {
public:
    AudioManager();
    ~AudioManager();
    void start();
    void stop();
    void toggleSound();
    void setSound(bool soundOn);
    static std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioInQueue;

    // API Interface
    constexpr std::string getInterfaceName() const override { return "AudioManager"; }
    HttpEndPointData_t getHttpEndpointData() override;

private:
    std::unique_ptr<AudioOutput> audioOutput;
    std::shared_ptr<SpeechIn> speechIn;
};