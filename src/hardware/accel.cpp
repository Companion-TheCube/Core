/*
 █████╗  ██████╗ ██████╗███████╗██╗         ██████╗██████╗ ██████╗
██╔══██╗██╔════╝██╔════╝██╔════╝██║        ██╔════╝██╔══██╗██╔══██╗
███████║██║     ██║     █████╗  ██║        ██║     ██████╔╝██████╔╝
██╔══██║██║     ██║     ██╔══╝  ██║        ██║     ██╔═══╝ ██╔═══╝
██║  ██║╚██████╗╚██████╗███████╗███████╗██╗╚██████╗██║     ██║
╚═╝  ╚═╝ ╚═════╝ ╚═════╝╚══════╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
This file implements the BMI270 accelerometer driver boundary for hardware wired directly to the Raspberry Pi.
The current implementation is intentionally a stub: it supports chip probing and raw acceleration reads while leaving
gesture feature programming for tap and lift detection as an explicit follow-up task.
*/

#include "accel.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <string>

namespace {

constexpr uint8_t kBmi270RegisterChipId = 0x00;
constexpr uint8_t kBmi270ExpectedChipId = 0x24;
constexpr uint8_t kBmi270RegisterAccelXLSB = 0x0C;
constexpr size_t kBmi270AccelerationPayloadLength = 6;
constexpr float kBmi270DefaultLsbPerG = 16384.0f; // +/-2g default scale assumption for the stub path.

std::expected<void, I2CError> ensureBus(const std::shared_ptr<ILocalI2CBus>& bus)
{
    if (!bus) {
        CubeLog::error("Bmi270Accelerometer has no I2C bus configured.");
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }
    return { };
}

Bmi270Error toBmi270Error(I2CError error)
{
    switch (error) {
    case I2CError::NOT_INITIALIZED:
        return Bmi270Error::NotInitialized;
    default:
        return Bmi270Error::I2cError;
    }
}

int16_t decodeLittleEndianI16(const I2CBytes& payload, size_t offset)
{
    return static_cast<int16_t>(
        static_cast<uint16_t>(payload[offset])
        | (static_cast<uint16_t>(payload[offset + 1]) << 8));
}

} // namespace

Bmi270Accelerometer::Bmi270Accelerometer(std::shared_ptr<ILocalI2CBus> bus, uint16_t address, bool tenBitAddress)
    : bus_(std::move(bus))
    , address_(address)
    , tenBitAddress_(tenBitAddress)
{
}

bool Bmi270Accelerometer::isConfigured() const
{
    return static_cast<bool>(bus_);
}

uint16_t Bmi270Accelerometer::address() const
{
    return address_;
}

std::expected<void, Bmi270Error> Bmi270Accelerometer::initialize() const
{
    const auto chipId = readChipId();
    if (!chipId) {
        return std::unexpected(chipId.error());
    }
    if (*chipId != kBmi270ExpectedChipId) {
        CubeLog::warning("Bmi270Accelerometer: unexpected chip ID " + std::to_string(*chipId) + ".");
        return std::unexpected(Bmi270Error::UnexpectedChipId);
    }

    CubeLog::info("Bmi270Accelerometer detected. Stub initialization completed without enabling gesture features.");
    return { };
}

std::expected<uint8_t, Bmi270Error> Bmi270Accelerometer::readChipId() const
{
    const auto response = readRegisters(kBmi270RegisterChipId, 1);
    if (!response) {
        return std::unexpected(toBmi270Error(response.error()));
    }
    if (response->size() != 1) {
        CubeLog::error("Bmi270Accelerometer chip ID read returned an unexpected payload length.");
        return std::unexpected(Bmi270Error::InvalidResponse);
    }
    return response->front();
}

std::expected<Bmi270AccelerationSample, Bmi270Error> Bmi270Accelerometer::readAcceleration() const
{
    const auto response = readRegisters(kBmi270RegisterAccelXLSB, kBmi270AccelerationPayloadLength);
    if (!response) {
        return std::unexpected(toBmi270Error(response.error()));
    }
    if (response->size() != kBmi270AccelerationPayloadLength) {
        CubeLog::error("Bmi270Accelerometer acceleration read returned an unexpected payload length.");
        return std::unexpected(Bmi270Error::InvalidResponse);
    }

    const int16_t rawX = decodeLittleEndianI16(*response, 0);
    const int16_t rawY = decodeLittleEndianI16(*response, 2);
    const int16_t rawZ = decodeLittleEndianI16(*response, 4);

    return Bmi270AccelerationSample {
        .xG = static_cast<float>(rawX) / kBmi270DefaultLsbPerG,
        .yG = static_cast<float>(rawY) / kBmi270DefaultLsbPerG,
        .zG = static_cast<float>(rawZ) / kBmi270DefaultLsbPerG,
    };
}

std::expected<void, Bmi270Error> Bmi270Accelerometer::configureInteractionDetection(const Bmi270InteractionConfig& config) const
{
    if (config.tapPeakThresholdG <= 0.0f
        || config.liftAccelerationDeltaThresholdG <= 0.0f
        || config.tapDebounceMs == 0
        || config.liftDebounceMs == 0) {
        return std::unexpected(Bmi270Error::InvalidConfig);
    }

    if (!config.tapDetectionEnabled && !config.liftDetectionEnabled) {
        return { };
    }

    CubeLog::warning("Bmi270Accelerometer gesture feature configuration is stubbed and not yet programmed into the BMI270.");
    return std::unexpected(Bmi270Error::NotImplemented);
}

std::expected<Bmi270InteractionStatus, Bmi270Error> Bmi270Accelerometer::pollInteractionStatus() const
{
    Bmi270InteractionStatus status;

    const auto acceleration = readAcceleration();
    if (!acceleration) {
        return std::unexpected(acceleration.error());
    }
    status.latestSample = *acceleration;

    // Gesture classification is intentionally left unresolved in the stub.
    status.tapDetected = false;
    status.liftState = Bmi270LiftState::Unknown;
    return status;
}

std::expected<void, I2CError> Bmi270Accelerometer::writeRegister(uint8_t reg, uint8_t value) const
{
    if (auto busReady = ensureBus(bus_); !busReady) {
        return busReady;
    }

    return bus_->write(address_, I2CBytes { reg, value }, tenBitAddress_);
}

std::expected<I2CBytes, I2CError> Bmi270Accelerometer::readRegisters(uint8_t reg, size_t length) const
{
    if (auto busReady = ensureBus(bus_); !busReady) {
        return std::unexpected(busReady.error());
    }

    return bus_->writeRead(address_, I2CBytes { reg }, length, tenBitAddress_);
}
