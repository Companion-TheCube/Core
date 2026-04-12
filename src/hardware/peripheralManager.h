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

Copyright (c) 2026 A-McD Technology LLC

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

#include "../api/api.h"
#include "io_bridge/ioBridge.h"
#include "mmWave.h"
#include "nfc.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

struct PresenceStatusSnapshot {
    MmWavePresenceState immediateState = MmWavePresenceState::Unknown;
    MmWavePresenceState delayedState = MmWavePresenceState::Unknown;
    int absentTimeoutSecs = 15;
};

class DelayedPresenceTracker {
public:
    explicit DelayedPresenceTracker(int absentTimeoutSecs = 15);

    void reset();
    void setAbsentTimeoutSecs(int absentTimeoutSecs, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    int absentTimeoutSecs() const;

    PresenceStatusSnapshot updateImmediateState(
        MmWavePresenceState immediateState,
        std::chrono::steady_clock::time_point now);
    PresenceStatusSnapshot snapshot(std::chrono::steady_clock::time_point now);

private:
    static int clampAbsentTimeoutSecs(int value);
    void reconcileDelayedState(std::chrono::steady_clock::time_point now);

    int absentTimeoutSecs_ = 15;
    MmWavePresenceState immediateState_ = MmWavePresenceState::Unknown;
    MmWavePresenceState delayedState_ = MmWavePresenceState::Unknown;
    std::optional<std::chrono::steady_clock::time_point> absentStateStartedAt_ {};
};

class PeripheralManager : public AutoRegisterAPI<PeripheralManager> {
private:
    std::unique_ptr<mmWave> mmWaveSensor;
    // NFC nfcSensor;
    // IMU imuSensor;
    mutable std::mutex presenceStatusMutex;
    DelayedPresenceTracker delayedPresenceTracker_;
    bool presenceDetectionEnabled_ = true;

    void syncPresenceConfigFromSettings();
    void syncPresenceAbsentTimeoutFromSettings();

protected:
    void syncPresenceDetectionEnabledFromSettings();
    void handleImmediatePresenceDecision(const MmWavePresenceDecision& decision);

public:
    PeripheralManager();
    explicit PeripheralManager(std::unique_ptr<mmWave> mmWaveSensorOverride, bool registerSettingCallbacks = true);
    ~PeripheralManager();

    std::string getInterfaceName() const override { return "Presence"; }
    HttpEndPointData_t getHttpEndpointData() override;

    bool isMmWavePresent();
    MmWavePresenceState getImmediatePresenceState();
    MmWavePresenceState getDelayedPresenceState();
    PresenceStatusSnapshot getPresenceStatus();
};

#endif // PERIPHERALMANAGER_H
