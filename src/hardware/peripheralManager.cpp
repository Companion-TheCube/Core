/*
██████╗ ███████╗██████╗ ██╗██████╗ ██╗  ██╗███████╗██████╗  █████╗ ██╗     ███╗   ███╗ █████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗
██╔══██╗██╔════╝██╔══██╗██║██╔══██╗██║  ██║██╔════╝██╔══██╗██╔══██╗██║     ████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██████╔╝█████╗  ██████╔╝██║██████╔╝███████║█████╗  ██████╔╝███████║██║     ██╔████╔██║███████║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔═══╝ ██╔══╝  ██╔══██╗██║██╔═══╝ ██╔══██║██╔══╝  ██╔══██╗██╔══██║██║     ██║╚██╔╝██║██╔══██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
██║     ███████╗██║  ██║██║██║     ██║  ██║███████╗██║  ██║██║  ██║███████╗██║ ╚═╝ ██║██║  ██║██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║
╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#include "peripheralManager.h"
#include "../gui/gui.h"
#include "../settings/globalSettings.h"
#include <chrono>
#include <thread>

std::function<void()> PeripheralManager::onMmWaveTuningRequested;

PeripheralManager::PeripheralManager()
{
    this->mmWaveSensor = std::make_unique<mmWave>();

    // Once the serial port opens, re-apply any saved calibration automatically.
    this->mmWaveSensor->setOnReadyCallback([this]() {
        if (GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::MMWAVE_IS_CALIBRATED)) {
            int movGate = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_MAX_MOVING_GATE);
            int restGate = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_MAX_RESTING_GATE);
            int unmanned = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_UNMANNED_DURATION_SECS);
            int motSens = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_MOTION_SENSITIVITY);
            int restSens = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_RESTING_SENSITIVITY);
            this->mmWaveSensor->configure(movGate, restGate, unmanned, motSens, restSens);
        }
    });
}

PeripheralManager::~PeripheralManager()
{
}

float PeripheralManager::getMmWavePresenceConfidence()
{
    if (!mmWaveSensor) {
        return 0.0f;
    }
    return mmWaveSensor->getPresenceConfidence();
}

bool PeripheralManager::isMmWavePresent()
{
    if (!mmWaveSensor) {
        return false;
    }
    return mmWaveSensor->isPresent();
}

void PeripheralManager::startTuning()
{
    if (onMmWaveTuningRequested) {
        onMmWaveTuningRequested();
    }
}

void PeripheralManager::applyCalibration(int unmannedSecs)
{
    if (!mmWaveTuner_ || !mmWaveSensor) {
        return;
    }

    MmWaveTuner::CalibrationResult result = mmWaveTuner_->analyze();

    GlobalSettings::setSetting(GlobalSettings::SettingType::MMWAVE_MAX_MOVING_GATE, result.maxMovingGate);
    GlobalSettings::setSetting(GlobalSettings::SettingType::MMWAVE_MAX_RESTING_GATE, result.maxRestingGate);
    GlobalSettings::setSetting(GlobalSettings::SettingType::MMWAVE_UNMANNED_DURATION_SECS, unmannedSecs);
    GlobalSettings::setSetting(GlobalSettings::SettingType::MMWAVE_MOTION_SENSITIVITY, result.motionSensitivity);
    GlobalSettings::setSetting(GlobalSettings::SettingType::MMWAVE_RESTING_SENSITIVITY, result.restingSensitivity);
    GlobalSettings::setSetting(GlobalSettings::SettingType::MMWAVE_IS_CALIBRATED, true);

    mmWaveSensor->configure(result.maxMovingGate, result.maxRestingGate, unmannedSecs,
        result.motionSensitivity, result.restingSensitivity);
}

void PeripheralManager::startMmWaveTuning()
{
    if (!mmWaveSensor) {
        CubeLog::error("PeripheralManager: mmWave sensor not initialised.");
        return;
    }

    // Allocate a fresh tuner for this session.
    mmWaveTuner_ = std::make_unique<MmWaveTuner>(mmWaveSensor.get());

    GUI::showNotificationWithCallback(
        "Presence Sensor Calibration",
        "Would you like to calibrate the presence sensor?\n\nThe wizard takes about 1 minute.",
        NotificationsManager::NotificationType::NOTIFICATION_OKAY,
        // ── YES ──────────────────────────────────────────────────────────────
        [this]() {
            GUI::suspendVisibleMenus();

            if (!mmWaveSensor->resetToDefaults()) {
                CubeLog::warning("Calibration: failed to reset sensor to defaults; continuing with current sensor configuration.");
            }

            auto showCountdownThen = std::make_shared<std::function<void(
                const std::string&, const std::string&, int, std::function<void()>)>>();

            *showCountdownThen = [showCountdownThen](
                                     const std::string& title,
                                     const std::string& instructions,
                                     int seconds,
                                     std::function<void()> onComplete) {
                auto tick = std::make_shared<std::function<void(int)>>();
                *tick = [title, instructions, onComplete, tick](int remaining) {
                    std::string message = instructions + "\n\nStarting in " + std::to_string(std::max(remaining, 0)) + "...";
                    GUI::showMessageBox(title, message, { 720, 720 }, { 0, 0 }, []() { });
                    if (remaining <= 0) {
                        if (onComplete) {
                            onComplete();
                        }
                        return;
                    }
                    std::thread([tick, remaining]() {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        (*tick)(remaining - 1);
                    }).detach();
                };
                (*tick)(seconds);
            };

            (*showCountdownThen)(
                "Step 1 of 3 — Prepare Empty Workspace",
                "Step 1 requires an empty room/workspace for 15 seconds.",
                10,
                [this, showCountdownThen]() {
                    GUI::showMessageBox(
                        "Step 1 of 3 — Analyzing Empty Workspace",
                        "Analyzing an empty room/workspace for 15 seconds...",
                        { 720, 720 }, { 0, 0 }, []() { });

                    mmWaveTuner_->startPhase(0, 15000, [this, showCountdownThen]() {
                        (*showCountdownThen)(
                            "Step 2 of 3 — Prepare Movement",
                            "Step 2 requires 15 seconds of movement around the room/workspace.",
                            10,
                            [this, showCountdownThen]() {
                                GUI::showMessageBox(
                                    "Step 2 of 3 — Analyzing Movement",
                                    "Analyzing movement around the room/workspace for 15 seconds...",
                                    { 720, 720 }, { 0, 0 }, []() { });

                                mmWaveTuner_->startPhase(1, 15000, [this, showCountdownThen]() {
                                    (*showCountdownThen)(
                                        "Step 3 of 3 — Prepare Seated Position",
                                        "Step 3 requires you to sit at your workspace in your normal work position for 15 seconds.",
                                        10,
                                        [this]() {
                                            GUI::showMessageBox(
                                                "Step 3 of 3 — Analyzing Seated Position",
                                                "Analyzing your seated workspace position for 15 seconds...",
                                                { 720, 720 }, { 0, 0 }, []() { });

                                            mmWaveTuner_->startPhase(2, 15000, [this]() {
                                                GUI::hideMessageBox();

                                                GUI::showSliderBox(
                                                    "Time Until Workspace Marked Vacant (seconds)",
                                                    5, 0, 300, 5,
                                                    [this](int secs) {
                                                        applyCalibration(secs);
                                                        GUI::showNotificationWithCallback(
                                                            "Calibration Complete",
                                                            "Presence sensor calibration is complete.\nSelect Approve to return to the menu.",
                                                            NotificationsManager::NotificationType::NOTIFICATION_OKAY,
                                                            []() { GUI::restoreSuspendedMenus(); },
                                                            []() { GUI::restoreSuspendedMenus(); });
                                                    },
                                                    []() {
                                                        GUI::showNotificationWithCallback(
                                                            "Calibration Cancelled",
                                                            "Sensor calibration was cancelled. Previous settings are unchanged.\nSelect Approve to return to the menu.",
                                                            NotificationsManager::NotificationType::NOTIFICATION_WARNING,
                                                            []() { GUI::restoreSuspendedMenus(); },
                                                            []() { GUI::restoreSuspendedMenus(); });
                                                    });
                                            });
                                        });
                                });
                            });
                    });
                });
        },
        // ── NO ───────────────────────────────────────────────────────────────
        []() {});
}