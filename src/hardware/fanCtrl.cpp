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
FanController currently exposes basic command and register access over the injected local I2C bus.
*/

#include "fanCtrl.h"

namespace {

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
