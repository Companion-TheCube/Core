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
#include "../settings/globalSettings.h"
#include <algorithm>

namespace {

int clampAverageWindowSeconds(int value)
{
    return std::clamp(value, 1, 30);
}

MmWavePresenceConfig loadPresenceConfigFromSettings()
{
    MmWavePresenceConfig config;
    config.detectionDistanceAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS));
    config.movingDistanceAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS));
    config.stationaryDistanceAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS));
    config.stationaryEnergyAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS));
    return config;
}

} // namespace

PeripheralManager::PeripheralManager()
{
    this->mmWaveSensor = std::make_unique<mmWave>();
    syncPresenceConfigFromSettings();

    const auto registerConfigCallback = [this](GlobalSettings::SettingType settingType) {
        GlobalSettings::setSettingCB(settingType, [this]() {
            syncPresenceConfigFromSettings();
        });
    };

    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS);
    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS);
    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS);
    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS);
}

PeripheralManager::~PeripheralManager()
{
}

void PeripheralManager::syncPresenceConfigFromSettings()
{
    if (!mmWaveSensor) {
        return;
    }

    mmWaveSensor->setPresenceConfig(loadPresenceConfigFromSettings());
}

bool PeripheralManager::isMmWavePresent()
{
    if (!mmWaveSensor) {
        return false;
    }
    return mmWaveSensor->isPresent();
}
