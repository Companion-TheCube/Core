/*
 █████╗  ██████╗ ██████╗███████╗██╗        ██╗  ██╗
██╔══██╗██╔════╝██╔════╝██╔════╝██║        ██║  ██║
███████║██║     ██║     █████╗  ██║        ███████║
██╔══██║██║     ██║     ██╔══╝  ██║        ██╔══██║
██║  ██║╚██████╗╚██████╗███████╗███████╗██╗██║  ██║
╚═╝  ╚═╝ ╚═════╝ ╚═════╝╚══════╝╚══════╝╚═╝╚═╝  ╚═╝
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

/*
This file defines the BMI270 accelerometer driver boundary for hardware wired directly to the Raspberry Pi.
The intent of this device is to expose enough structure for future tap and lift-off-desk detection without locking
the project into a finished gesture algorithm yet.
*/

#pragma once
#ifndef ACCEL_H
#define ACCEL_H

#include "pi_i2c.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>

enum class Bmi270Error : uint8_t {
    NotInitialized = 0,
    InvalidConfig,
    InvalidResponse,
    UnexpectedChipId,
    NotImplemented,
    I2cError,
};

struct Bmi270AccelerationSample {
    float xG = 0.0f;
    float yG = 0.0f;
    float zG = 0.0f;
};

enum class Bmi270LiftState : uint8_t {
    Unknown = 0,
    OnDesk,
    Lifted,
};

struct Bmi270InteractionConfig {
    bool tapDetectionEnabled = true;
    bool liftDetectionEnabled = true;
    float tapPeakThresholdG = 0.8f;
    float liftAccelerationDeltaThresholdG = 0.25f;
    uint16_t tapDebounceMs = 100;
    uint16_t liftDebounceMs = 150;
};

struct Bmi270InteractionStatus {
    bool tapDetected = false;
    Bmi270LiftState liftState = Bmi270LiftState::Unknown;
    std::optional<Bmi270AccelerationSample> latestSample;
};

class Bmi270Accelerometer {
public:
    Bmi270Accelerometer(std::shared_ptr<ILocalI2CBus> bus, uint16_t address = 0x68, bool tenBitAddress = false);

    bool isConfigured() const;
    uint16_t address() const;

    std::expected<void, Bmi270Error> initialize() const;
    std::expected<uint8_t, Bmi270Error> readChipId() const;
    std::expected<Bmi270AccelerationSample, Bmi270Error> readAcceleration() const;
    std::expected<void, Bmi270Error> configureInteractionDetection(const Bmi270InteractionConfig& config) const;
    std::expected<Bmi270InteractionStatus, Bmi270Error> pollInteractionStatus() const;

private:
    std::expected<void, I2CError> writeRegister(uint8_t reg, uint8_t value) const;
    std::expected<I2CBytes, I2CError> readRegisters(uint8_t reg, size_t length) const;

    std::shared_ptr<ILocalI2CBus> bus_;
    uint16_t address_ = 0;
    bool tenBitAddress_ = false;
};

#endif
