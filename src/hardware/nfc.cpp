/*
███╗   ██╗███████╗ ██████╗    ██████╗██████╗ ██████╗
████╗  ██║██╔════╝██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██╔██╗ ██║█████╗  ██║        ██║     ██████╔╝██████╔╝
██║╚██╗██║██╔══╝  ██║        ██║     ██╔═══╝ ██╔═══╝
██║ ╚████║██║     ╚██████╗██╗╚██████╗██║     ██║
╚═╝  ╚═══╝╚═╝      ╚═════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
This file implements the NFC driver boundary for hardware wired directly to the Raspberry Pi.
NfcDevice currently provides a thin I2C transport surface that can be backed by real or test bus implementations.
*/

#include "nfc.h"

namespace {

std::expected<void, I2CError> ensureBus(const std::shared_ptr<ILocalI2CBus>& bus)
{
    if (!bus) {
        CubeLog::error("NfcDevice has no I2C bus configured.");
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }
    return { };
}

} // namespace

NfcDevice::NfcDevice(std::shared_ptr<ILocalI2CBus> bus, uint16_t address, bool tenBitAddress)
    : bus_(std::move(bus))
    , address_(address)
    , tenBitAddress_(tenBitAddress)
{
}

bool NfcDevice::isConfigured() const
{
    return static_cast<bool>(bus_);
}

uint16_t NfcDevice::address() const
{
    return address_;
}

std::expected<void, I2CError> NfcDevice::writeFrame(const I2CBytes& frame) const
{
    if (auto busReady = ensureBus(bus_); !busReady) {
        return busReady;
    }

    return bus_->write(address_, frame, tenBitAddress_);
}

std::expected<I2CBytes, I2CError> NfcDevice::transceive(const I2CBytes& txFrame, size_t rxLen) const
{
    if (auto busReady = ensureBus(bus_); !busReady) {
        return std::unexpected(busReady.error());
    }

    return bus_->writeRead(address_, txFrame, rxLen, tenBitAddress_);
}
