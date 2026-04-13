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

#include "accel.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <cmath>
#include <string>

namespace {

constexpr uint8_t kBmi270RegisterChipId = 0x00;
constexpr uint8_t kBmi270ExpectedChipId = 0x24;
constexpr uint8_t kBmi270RegisterAccelXLSB = 0x0C;
constexpr size_t kBmi270AccelerationPayloadLength = 6;
constexpr float kBmi270LsbPerG = 8192.0f; // +/-4g full-scale.
constexpr float kRestMagnitudeTargetG = 1.0f;
constexpr float kNoMotionScaleFactor = 0.35f;
constexpr float kMinimumNoMotionThresholdG = 0.05f;

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

float sampleMagnitude(const Bmi270AccelerationSample& sample)
{
    return std::sqrt((sample.xG * sample.xG) + (sample.yG * sample.yG) + (sample.zG * sample.zG));
}

float sampleDistance(const Bmi270AccelerationSample& left, const Bmi270AccelerationSample& right)
{
    const float dx = left.xG - right.xG;
    const float dy = left.yG - right.yG;
    const float dz = left.zG - right.zG;
    return std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
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

bool Bmi270Accelerometer::isInitialized() const
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    return initialized_;
}

bool Bmi270Accelerometer::isAvailable() const
{
    std::lock_guard<std::mutex> lock(stateMutex_);
    return isConfigured() && !unavailable_;
}

uint16_t Bmi270Accelerometer::address() const
{
    return address_;
}

std::expected<void, Bmi270Error> Bmi270Accelerometer::initialize()
{
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (initialized_) {
            return { };
        }
        if (unavailable_) {
            return std::unexpected(Bmi270Error::Unavailable);
        }
    }

    const auto chipId = readChipId();
    if (!chipId) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        unavailable_ = true;
        return std::unexpected(chipId.error());
    }

    if (*chipId != kBmi270ExpectedChipId) {
        CubeLog::warning("Bmi270Accelerometer: unexpected chip ID " + std::to_string(*chipId) + ".");
        std::lock_guard<std::mutex> lock(stateMutex_);
        unavailable_ = true;
        return std::unexpected(Bmi270Error::UnexpectedChipId);
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    initialized_ = true;
    unavailable_ = false;
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

std::expected<Bmi270AccelerationSample, Bmi270Error> Bmi270Accelerometer::readAcceleration()
{
    if (auto initResult = initialize(); !initResult) {
        return std::unexpected(initResult.error());
    }

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
        .xG = static_cast<float>(rawX) / kBmi270LsbPerG,
        .yG = static_cast<float>(rawY) / kBmi270LsbPerG,
        .zG = static_cast<float>(rawZ) / kBmi270LsbPerG,
    };
}

std::expected<void, Bmi270Error> Bmi270Accelerometer::configureInteractionDetection(const Bmi270InteractionConfig& config)
{
    if (config.tapPeakThresholdG <= 0.0f
        || config.liftAccelerationDeltaThresholdG <= 0.0f
        || config.tapDebounceMs == 0) {
        return std::unexpected(Bmi270Error::InvalidConfig);
    }

    if (auto initResult = initialize(); !initResult) {
        return std::unexpected(initResult.error());
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    const bool configChanged = !interactionConfigured_
        || interactionConfig_.tapDetectionEnabled != config.tapDetectionEnabled
        || interactionConfig_.liftDetectionEnabled != config.liftDetectionEnabled
        || interactionConfig_.tapPeakThresholdG != config.tapPeakThresholdG
        || interactionConfig_.liftAccelerationDeltaThresholdG != config.liftAccelerationDeltaThresholdG
        || interactionConfig_.tapDebounceMs != config.tapDebounceMs;
    interactionConfig_ = config;
    interactionConfigured_ = config.tapDetectionEnabled || config.liftDetectionEnabled;
    if (configChanged) {
        previousSample_.reset();
        lastTapAt_.reset();
    }
    return { };
}

std::expected<Bmi270InterruptStatus, Bmi270Error> Bmi270Accelerometer::readInterruptStatus()
{
    const auto status = pollInteractionStatus();
    if (!status) {
        return std::unexpected(status.error());
    }

    return status->interruptStatus;
}

std::expected<Bmi270InteractionStatus, Bmi270Error> Bmi270Accelerometer::pollInteractionStatus()
{
    const auto sample = readAcceleration();
    if (!sample) {
        return std::unexpected(sample.error());
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (!interactionConfigured_) {
            interactionConfig_ = Bmi270InteractionConfig {};
            interactionConfigured_ = true;
        }
    }

    return classifyInteraction(*sample);
}

Bmi270InteractionStatus Bmi270Accelerometer::classifyInteraction(const Bmi270AccelerationSample& sample)
{
    Bmi270InteractionStatus status;
    status.latestSample = sample;

    std::lock_guard<std::mutex> lock(stateMutex_);

    const float magnitude = sampleMagnitude(sample);
    const float restDelta = std::fabs(magnitude - kRestMagnitudeTargetG);
    const float deltaFromPrevious = previousSample_.has_value()
        ? sampleDistance(sample, *previousSample_)
        : 0.0f;

    if (interactionConfig_.liftDetectionEnabled) {
        status.interruptStatus.motionDetected = deltaFromPrevious >= interactionConfig_.liftAccelerationDeltaThresholdG;
        status.interruptStatus.noMotionDetected = deltaFromPrevious <= std::max(
            interactionConfig_.liftAccelerationDeltaThresholdG * kNoMotionScaleFactor,
            kMinimumNoMotionThresholdG);
    }

    if (interactionConfig_.tapDetectionEnabled) {
        const auto now = std::chrono::steady_clock::now();
        const bool debounceExpired = !lastTapAt_.has_value()
            || std::chrono::duration_cast<std::chrono::milliseconds>(now - *lastTapAt_).count() >= interactionConfig_.tapDebounceMs;

        if (debounceExpired
            && (restDelta + deltaFromPrevious) >= interactionConfig_.tapPeakThresholdG) {
            status.interruptStatus.tapDetected = true;
            lastTapAt_ = now;
        }
    }

    status.interruptStatus.rawStatus
        = static_cast<uint16_t>((status.interruptStatus.tapDetected ? 0x0001U : 0U)
            | (status.interruptStatus.motionDetected ? 0x0002U : 0U)
            | (status.interruptStatus.noMotionDetected ? 0x0004U : 0U));

    previousSample_ = sample;
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
