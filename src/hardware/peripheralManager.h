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
#include "accel.h"
#include "fanCtrl.h"
#include "hardwareInfo.h"
#include "interactionEvents.h"
#include "io_bridge/ioBridge.h"
#include "mmWave.h"
#include "nfc.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

struct PresenceStatusSnapshot {
    MmWavePresenceState immediateState = MmWavePresenceState::Unknown;
    MmWavePresenceState delayedState = MmWavePresenceState::Unknown;
    int absentTimeoutSecs = 15;
};

struct ThermalStatusSnapshot {
    bool controlEnabled = true;
    bool fanConfigured = false;
    bool failsafeActive = false;
    std::optional<double> cpuTemperatureC;
    std::optional<double> systemTemperatureC;
    uint8_t appliedDutyPercent = 0;
};

struct InteractionStatusSnapshot {
    bool available = false;
    bool enabled = true;
    bool initialized = false;
    Bmi270LiftState liftState = Bmi270LiftState::Unknown;
    std::optional<Bmi270AccelerationSample> latestSample;
    std::optional<uint64_t> lastTapAtEpochMs;
    std::optional<uint64_t> lastLiftChangeAtEpochMs;
    uint64_t lastEventSequence = 0;
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
public:
    using TemperatureReader = std::function<std::optional<double>()>;
    using MonotonicNowReader = std::function<std::chrono::steady_clock::time_point()>;
    using EpochMsReader = std::function<uint64_t()>;

private:
    struct SettingsCallbackGate {
        PeripheralManager* owner = nullptr;
    };

    std::unique_ptr<mmWave> mmWaveSensor_;
    std::unique_ptr<FanController> fanController_;
    std::unique_ptr<Bmi270Accelerometer> accelerometer_;

    TemperatureReader cpuTemperatureReader_;
    TemperatureReader systemTemperatureReader_;
    MonotonicNowReader monotonicNowReader_;
    EpochMsReader epochMsReader_;

    mutable std::mutex presenceStatusMutex_;
    mutable std::mutex thermalStatusMutex_;
    mutable std::mutex interactionStatusMutex_;

    DelayedPresenceTracker delayedPresenceTracker_;
    bool presenceDetectionEnabled_ = true;
    ThermalStatusSnapshot thermalStatus_;
    InteractionStatusSnapshot interactionStatus_;
    std::optional<double> lastAppliedDutyTemperatureC_;
    std::atomic<uint64_t> nextInteractionSequence_ { 1 };
    std::optional<Bmi270AccelerationSample> deskBaselineSample_;
    std::optional<std::chrono::steady_clock::time_point> stableSince_;
    std::optional<std::chrono::steady_clock::time_point> liftCandidateSince_;

    std::jthread thermalControlThread_;
    std::condition_variable thermalControlWakeCv_;
    std::mutex thermalControlWakeMutex_;
    bool thermalControlWakeRequested_ = false;

    std::jthread interactionControlThread_;
    std::condition_variable interactionControlWakeCv_;
    std::mutex interactionControlWakeMutex_;
    bool interactionControlWakeRequested_ = false;

    std::shared_ptr<SettingsCallbackGate> settingsCallbackGate_;

    void syncPresenceConfigFromSettings();
    void syncPresenceAbsentTimeoutFromSettings();
    void registerSettingsCallbacks();

    void startThermalControlLoop();
    void thermalControlLoop(std::stop_token stopToken);
    void setThermalStatusSnapshot(const ThermalStatusSnapshot& status, std::optional<double> appliedDutyTemperatureC = std::nullopt);
    std::expected<void, I2CError> applyFanDutyPercent(uint8_t dutyPercent);

    void startInteractionControlLoop();
    void interactionControlLoop(std::stop_token stopToken);
    void setInteractionStatusSnapshot(const InteractionStatusSnapshot& status);
    InteractionEvent recordInteractionEvent(
        InteractionEventType type,
        Bmi270LiftState liftStateAfter,
        const std::optional<Bmi270AccelerationSample>& sample,
        uint64_t occurredAtEpochMs);

protected:
    void syncPresenceDetectionEnabledFromSettings();
    void handleImmediatePresenceDecision(const MmWavePresenceDecision& decision);
    void runThermalControlIteration();
    void requestThermalControlWake();

    void runInteractionControlIteration();
    void requestInteractionControlWake();

public:
    PeripheralManager();
    explicit PeripheralManager(std::unique_ptr<mmWave> mmWaveSensorOverride, bool registerSettingCallbacks = true);
    PeripheralManager(
        std::unique_ptr<mmWave> mmWaveSensorOverride,
        std::unique_ptr<FanController> fanControllerOverride,
        TemperatureReader cpuTemperatureReader = {},
        TemperatureReader systemTemperatureReader = {},
        bool registerSettingCallbacks = true,
        bool startThermalControlLoopThread = true);
    PeripheralManager(
        std::unique_ptr<mmWave> mmWaveSensorOverride,
        std::unique_ptr<FanController> fanControllerOverride,
        std::unique_ptr<Bmi270Accelerometer> accelerometerOverride,
        TemperatureReader cpuTemperatureReader = {},
        TemperatureReader systemTemperatureReader = {},
        MonotonicNowReader monotonicNowReader = {},
        EpochMsReader epochMsReader = {},
        bool registerSettingCallbacks = true,
        bool startThermalControlLoopThread = true,
        bool startInteractionControlLoopThread = true);
    ~PeripheralManager();

    std::string getInterfaceName() const override { return "Presence"; }
    HttpEndPointData_t getHttpEndpointData() override;

    bool isMmWavePresent();
    MmWavePresenceState getImmediatePresenceState();
    MmWavePresenceState getDelayedPresenceState();
    PresenceStatusSnapshot getPresenceStatus();
    ThermalStatusSnapshot getThermalStatus() const;
    InteractionStatusSnapshot getInteractionStatus() const;
};

#endif // PERIPHERALMANAGER_H
