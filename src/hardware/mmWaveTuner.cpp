/*
███╗   ███╗███╗   ███╗██╗    ██╗ █████╗ ██╗   ██╗███████╗   ████████╗██╗   ██╗███╗   ██╗███████╗██████╗
████╗ ████║████╗ ████║██║    ██║██╔══██╗██║   ██║██╔════╝   ╚══██╔══╝██║   ██║████╗  ██║██╔════╝██╔══██╗
██╔████╔██║██╔████╔██║██║ █╗ ██║███████║██║   ██║█████╗        ██║   ██║   ██║██╔██╗ ██║█████╗  ██████╔╝
██║╚██╔╝██║██║╚██╔╝██║██║███╗██║██╔══██║╚██╗ ██╔╝██╔══╝        ██║   ██║   ██║██║╚██╗██║██╔══╝  ██╔══██╗
██║ ╚═╝ ██║██║ ╚═╝ ██║╚███╔███╔╝██║  ██║ ╚████╔╝ ███████╗██╗   ██║   ╚██████╔╝██║ ╚████║███████╗██║  ██║
╚═╝     ╚═╝╚═╝     ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝
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

#include "mmWaveTuner.h"
#include <algorithm>
#include <chrono>
#include <thread>

MmWaveTuner::MmWaveTuner(mmWave* sensor)
    : sensor_(sensor)
{
}

MmWaveTuner::~MmWaveTuner()
{
    cancel();
}

void MmWaveTuner::startPhase(int phaseIndex, int durationMs, std::function<void()> onComplete)
{
    cancel();

    collectionThread_ = std::make_unique<std::jthread>([this, phaseIndex, durationMs, onComplete](std::stop_token st) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(durationMs);
        while (!st.stop_requested() && std::chrono::steady_clock::now() < deadline) {
            MmWaveReading reading = sensor_->getReading();
            if (phaseIndex == 0) {
                collectedSamples_.emptySamples.push_back(reading);
            } else if (phaseIndex == 1) {
                collectedSamples_.movingSamples.push_back(reading);
            } else if (phaseIndex == 2) {
                collectedSamples_.restingSamples.push_back(reading);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!st.stop_requested()) {
            // Dispatch the callback onto a fresh thread so this jthread can exit
            // cleanly before startPhase() (called from onComplete) tries to join it.
            std::thread(std::move(onComplete)).detach();
        }
    });
}

void MmWaveTuner::cancel()
{
    if (collectionThread_) {
        collectionThread_->request_stop();
        collectionThread_.reset();
    }
}

MmWaveTuner::CalibrationResult MmWaveTuner::analyze() const
{
    return analyzeMmWaveCalibrationSamples(collectedSamples_);
}
