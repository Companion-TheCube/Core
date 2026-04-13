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
The Bosch BMI270 vendor package is fetched during CMake configure so the project has a stable place to land
the official support package, but this wrapper keeps the rest of CORE isolated from the raw transport details.
*/

#pragma once
#ifndef ACCEL_H
#define ACCEL_H

#include "pi_i2c.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <optional>

enum class Bmi270Error : uint8_t {
    NotInitialized = 0,
    InvalidConfig,
    InvalidResponse,
    UnexpectedChipId,
    Unavailable,
    I2cError,
};

struct Bmi270AccelerationSample {
    float xG = 0.0f;
    float yG = 0.0f;
    float zG = 0.0f;
};

struct Bmi270InterruptStatus {
    bool tapDetected = false;
    bool motionDetected = false;
    bool noMotionDetected = false;
    uint16_t rawStatus = 0;
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
    float liftAccelerationDeltaThresholdG = 0.20f;
    uint16_t tapDebounceMs = 120;
};

struct Bmi270InteractionStatus {
    Bmi270InterruptStatus interruptStatus;
    std::optional<Bmi270AccelerationSample> latestSample;
};

class Bmi270Accelerometer {
public:
    Bmi270Accelerometer(std::shared_ptr<ILocalI2CBus> bus, uint16_t address = 0x68, bool tenBitAddress = false);
    virtual ~Bmi270Accelerometer() = default;

    virtual bool isConfigured() const;
    virtual bool isInitialized() const;
    virtual bool isAvailable() const;
    virtual uint16_t address() const;

    virtual std::expected<void, Bmi270Error> initialize();
    virtual std::expected<uint8_t, Bmi270Error> readChipId() const;
    virtual std::expected<Bmi270AccelerationSample, Bmi270Error> readAcceleration();
    virtual std::expected<Bmi270InterruptStatus, Bmi270Error> readInterruptStatus();
    virtual std::expected<void, Bmi270Error> configureInteractionDetection(const Bmi270InteractionConfig& config);
    virtual std::expected<Bmi270InteractionStatus, Bmi270Error> pollInteractionStatus();

protected:
    std::expected<void, I2CError> writeRegister(uint8_t reg, uint8_t value) const;
    std::expected<I2CBytes, I2CError> readRegisters(uint8_t reg, size_t length) const;

private:
    Bmi270InteractionStatus classifyInteraction(const Bmi270AccelerationSample& sample);

    std::shared_ptr<ILocalI2CBus> bus_;
    uint16_t address_ = 0;
    bool tenBitAddress_ = false;

    mutable std::mutex stateMutex_;
    bool initialized_ = false;
    bool unavailable_ = false;
    bool interactionConfigured_ = false;
    Bmi270InteractionConfig interactionConfig_ {};
    std::optional<Bmi270AccelerationSample> previousSample_ {};
    std::optional<std::chrono::steady_clock::time_point> lastTapAt_ {};
};

#endif
