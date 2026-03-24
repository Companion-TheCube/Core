/*
██████╗ ███████╗██████╗ ██╗██████╗ ██╗  ██╗███████╗██████╗  █████╗ ██╗     ███╗   ███╗ █████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗    ██╗  ██╗
██╔══██╗██╔════╝██╔══██╗██║██╔══██╗██║  ██║██╔════╝██╔══██╗██╔══██╗██║     ████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗   ██║  ██║
██████╔╝█████╗  ██████╔╝██║██████╔╝███████║█████╗  ██████╔╝███████║██║     ██╔████╔██║███████║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝   ███████║
██╔═══╝ ██╔══╝  ██╔══██╗██║██╔═══╝ ██╔══██║██╔══╝  ██╔══██╗██╔══██║██║     ██║╚██╔╝██║██╔══██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗   ██╔══██║
██║     ███████╗██║  ██║██║██║     ██║  ██║███████╗██║  ██║██║  ██║███████╗██║ ╚═╝ ██║██║  ██║██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║██╗██║  ██║
╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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
#ifndef PERIPHERALMANAGER_H
#define PERIPHERALMANAGER_H

#include "io_bridge/ioBridge.h"
#include "mmWave.h"
#include "mmWaveTuner.h"
#include "nfc.h"
#include <functional>
#include <memory>

class PeripheralManager {
private:
    std::unique_ptr<mmWave> mmWaveSensor;
    std::unique_ptr<MmWaveTuner> mmWaveTuner_;
    // NFC nfcSensor;
    // IMU imuSensor;

    void applyCalibration(int unmannedSecs);

public:
    PeripheralManager();
    ~PeripheralManager();

    float getMmWavePresenceConfidence();
    bool isMmWavePresent();

    // Launch the three-phase GUI calibration wizard.
    void startMmWaveTuning();

    // Static callback invoked by the sensors menu "Calibrate" button.
    // Register this in main.cpp after both GUI and PeripheralManager are created.
    static std::function<void()> onMmWaveTuningRequested;
    static void startTuning();
};

#endif // PERIPHERALMANAGER_H
