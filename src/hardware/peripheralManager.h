#pragma once
#ifndef PERIPHERALMANAGER_H
#define PERIPHERALMANAGER_H

#include "mmWave.h"
#include "nfc.h"
#include "imu.h"
#include <memory>

class PeripheralManager {
private:
    std::unique_ptr<mmWave> mmWaveSensor;
    // NFC nfcSensor;
    // IMU imuSensor;
public:
    PeripheralManager();
    ~PeripheralManager();
};

#endif// PERIPHERALMANAGER_H
