/*
███████╗ █████╗ ███╗   ██╗ ██████╗████████╗██████╗ ██╗         ██████╗██████╗ ██████╗
██╔════╝██╔══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗██║        ██╔════╝██╔══██╗██╔══██╗
█████╗  ███████║██╔██╗ ██║██║        ██║   ██████╔╝██║        ██║     ██████╔╝██████╔╝
██╔══╝  ██╔══██║██║╚██╗██║██║        ██║   ██╔══██╗██║        ██║     ██╔═══╝ ██╔═══╝
██║     ██║  ██║██║ ╚████║╚██████╗   ██║   ██║  ██║███████╗██╗╚██████╗██║     ██║
╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
This file implements the fan-controller driver boundary for hardware wired directly to the Raspberry Pi.
FanController implements the logical CORE-to-controller I2C contract used for
TheCube thermal management. This protocol is defined here until the final fan
controller firmware is fixed.

Logical protocol definition
---------------------------
Read registers:
- 0x00: protocol version
- 0x01: status flags
- 0x02: target duty percent
- 0x03-0x04: tach RPM, little-endian
- 0x05: fault flags

Status flags:
- bit 0: controller enabled
- bit 1: manual-duty mode active
- bit 2: tach value valid
- bit 3: fault present

Write commands:
- 0x01 [0|1]: set enabled
- 0x02 [0=disabled,1=manual_duty]: set control mode
- 0x03 [0..100]: set target duty percent
*/

#include "fanCtrl.h"

#include <algorithm>

namespace {

constexpr uint8_t kFanRegisterProtocolVersion = 0x00;
constexpr uint8_t kFanRegisterStatusFlags = 0x01;
constexpr uint8_t kFanRegisterTargetDutyPercent = 0x02;
constexpr uint8_t kFanRegisterTachRpm = 0x03;
constexpr uint8_t kFanRegisterFaultFlags = 0x05;

constexpr uint8_t kFanCommandSetEnabled = 0x01;
constexpr uint8_t kFanCommandSetMode = 0x02;
constexpr uint8_t kFanCommandSetTargetDutyPercent = 0x03;

constexpr uint8_t kFanStatusEnabledBit = 0x01;
constexpr uint8_t kFanStatusManualDutyBit = 0x02;
constexpr uint8_t kFanStatusTachValidBit = 0x04;

std::expected<void, I2CError> ensureBus(const std::shared_ptr<ILocalI2CBus>& bus)
{
    if (!bus) {
        CubeLog::error("FanController has no I2C bus configured.");
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }
    return { };
}

} // namespace

FanController::FanController(std::shared_ptr<ILocalI2CBus> bus, uint16_t address, bool tenBitAddress)
    : bus_(std::move(bus))
    , address_(address)
    , tenBitAddress_(tenBitAddress)
{
}

bool FanController::isConfigured() const
{
    return static_cast<bool>(bus_);
}

std::expected<void, I2CError> FanController::setEnabled(bool enabled) const
{
    return writeCommand(kFanCommandSetEnabled, I2CBytes { static_cast<unsigned char>(enabled ? 1 : 0) });
}

std::expected<void, I2CError> FanController::setControlMode(FanControlMode mode) const
{
    return writeCommand(kFanCommandSetMode, I2CBytes { static_cast<unsigned char>(mode) });
}

std::expected<void, I2CError> FanController::setManualDutyPercent(uint8_t dutyPercent) const
{
    dutyPercent = static_cast<uint8_t>(std::min<uint8_t>(dutyPercent, 100));
    return writeCommand(kFanCommandSetTargetDutyPercent, I2CBytes { static_cast<unsigned char>(dutyPercent) });
}

std::expected<void, I2CError> FanController::writeCommand(uint8_t command, const I2CBytes& payload) const
{
    if (auto busReady = ensureBus(bus_); !busReady) {
        return busReady;
    }

    I2CBytes txFrame;
    txFrame.reserve(payload.size() + 1);
    txFrame.push_back(command);
    txFrame.insert(txFrame.end(), payload.begin(), payload.end());

    return bus_->write(address_, txFrame, tenBitAddress_);
}

std::expected<I2CBytes, I2CError> FanController::readRegister(uint8_t reg, size_t length) const
{
    if (auto busReady = ensureBus(bus_); !busReady) {
        return std::unexpected(busReady.error());
    }

    return bus_->writeRead(address_, I2CBytes { reg }, length, tenBitAddress_);
}

std::expected<FanStatus, I2CError> FanController::readStatus() const
{
    auto rawStatus = readRegister(kFanRegisterProtocolVersion, 6);
    if (!rawStatus) {
        return std::unexpected(rawStatus.error());
    }
    if (rawStatus->size() < 6) {
        CubeLog::error("FanController status read returned fewer bytes than expected.");
        return std::unexpected(I2CError::IO_FAILED);
    }

    FanStatus status;
    status.protocolVersion = (*rawStatus)[kFanRegisterProtocolVersion];
    status.statusFlags = (*rawStatus)[kFanRegisterStatusFlags];
    status.targetDutyPercent = static_cast<uint8_t>(std::min<unsigned char>((*rawStatus)[kFanRegisterTargetDutyPercent], 100));
    status.enabled = (status.statusFlags & kFanStatusEnabledBit) != 0;
    status.mode = (status.statusFlags & kFanStatusManualDutyBit) != 0
        ? FanControlMode::ManualDuty
        : FanControlMode::Disabled;

    if ((status.statusFlags & kFanStatusTachValidBit) != 0) {
        status.tachRpm = static_cast<uint16_t>(
            static_cast<uint16_t>((*rawStatus)[kFanRegisterTachRpm])
            | (static_cast<uint16_t>((*rawStatus)[kFanRegisterTachRpm + 1]) << 8));
    }

    status.faultFlags = (*rawStatus)[kFanRegisterFaultFlags];
    return status;
}

std::expected<uint8_t, I2CError> FanController::readProtocolVersion() const
{
    auto protocolVersion = readRegister(kFanRegisterProtocolVersion, 1);
    if (!protocolVersion) {
        return std::unexpected(protocolVersion.error());
    }
    if (protocolVersion->empty()) {
        CubeLog::error("FanController protocol version read returned no bytes.");
        return std::unexpected(I2CError::IO_FAILED);
    }
    return protocolVersion->front();
}
