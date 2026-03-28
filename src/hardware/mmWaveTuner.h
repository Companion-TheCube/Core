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

#pragma once
#include "mmWaveCalibration.h"
#include "mmWave.h"
#include <functional>
#include <memory>
#include <thread>
#include <vector>

// MmWaveTuner drives a three-phase sensor calibration procedure.
//
// Phase indices:
//   0 — Baseline: room should be empty. Samples are used to derive noise floors.
//   1 — Movement: user moves around normally. Samples are used to size the moving zone.
//   2 — Stationary: user sits/stands still. Samples are used to learn desk distance and resting tuning.
//
// After all phases finish, call analyze() to obtain recommended hardware + software values.
class MmWaveTuner {
public:
    using CalibrationResult = MmWaveCalibrationResult;

    explicit MmWaveTuner(mmWave* sensor);
    ~MmWaveTuner();

    // Begin a collection phase.  Samples are gathered for durationMs milliseconds on
    // a background thread, then onComplete is invoked on that thread.
    // Calling startPhase() while a phase is running cancels the current phase first.
    void startPhase(int phaseIndex, int durationMs, std::function<void()> onComplete);

    // Stop any running phase immediately.
    void cancel();

    // Compute recommended configuration from collected samples.
    // Safe to call from any thread after phases complete.
    CalibrationResult analyze() const;

private:
    mmWave* sensor_;
    std::unique_ptr<std::jthread> collectionThread_;

    MmWaveCalibrationSamples collectedSamples_;
};
