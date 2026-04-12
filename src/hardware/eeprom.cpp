/*
███████╗███████╗██████╗ ██████╗  ██████╗ ███╗   ███╗    ██████╗██████╗ ██████╗
██╔════╝██╔════╝██╔══██╗██╔══██╗██╔═══██╗████╗ ████║   ██╔════╝██╔══██╗██╔══██╗
█████╗  █████╗  ██████╔╝██████╔╝██║   ██║██╔████╔██║   ██║     ██████╔╝██████╔╝
██╔══╝  ██╔══╝  ██╔═══╝ ██╔══██╗██║   ██║██║╚██╔╝██║   ██║     ██╔═══╝ ██╔═══╝
███████╗███████╗██║     ██║  ██║╚██████╔╝██║ ╚═╝ ██║██╗╚██████╗██║     ██║
╚══════╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
This file implements the EEPROM driver boundary for hardware wired directly to the Raspberry Pi.
EepromDevice handles offset-prefixed reads and writes over the injected local I2C bus so the driver can be tested without hardware.

Planned logical EEPROM contents
===============================

TheCube EEPROM is intended to hold factory provisioning data and a small amount
of service / recovery state. The expectation is that roughly half of the
address space is write-protected after factory flashing, while the remaining
half stays writable for controlled field updates.

Write-protected region after factory flashing
---------------------------------------------
- Layout header:
  - EEPROM format version
  - total size / region sizes
  - CRCs or other integrity markers for each region
- Device identity:
  - serial number
  - model / SKU
  - hardware revision
  - manufacturing lot / batch
  - manufacturing date or timestamp
- Factory image metadata:
  - factory image / recovery image version
  - build number used during factory flash
  - provisioning revision
- Hardware capability map:
  - installed storage configuration
  - presence of mmWave, NFC, IMU, bridge board, etc.
  - board-specific capability flags and fitted-option bitmasks
  - bridge or coprocessor board revision where applicable
- Factory calibration / tuning:
  - mmWave installation or calibration data
  - audio-front-end defaults such as baseline mic gain or clipping thresholds
  - any per-board calibration blobs required for reliable bring-up
- Device trust bootstrap:
  - per-device secret, seed, or key material used to derive recovery /
    provisioning credentials
  - do not rely on storing long-lived plaintext operational passwords here

Writable region
---------------
- Recovery / provisioning connectivity:
  - WiFi SSID and password printed on the device sticker
  - optional recovery AP/static-IP provisioning parameters
- Service metadata:
  - factory reset counter
  - last factory reset timestamp
  - last service / rework timestamp
  - calibration blob version currently applied
- Last-known-good provisioning state:
  - provisioning completion marker
  - schema migration marker
  - recovery credentials revision

Intentionally not EEPROM-backed
-------------------------------
- Everyday user preferences such as volume, brightness, developer mode,
  personality toggles, and other mutable runtime settings. Those belong in the
  normal settings store so a factory reset can return them to defaults without
  rewriting immutable factory data.
*/

#include "eeprom.h"

EepromDevice::EepromDevice(
    std::shared_ptr<ILocalI2CBus> bus,
    uint16_t address,
    size_t addressWidthBytes,
    bool tenBitAddress)
    : bus_(std::move(bus))
    , address_(address)
    , addressWidthBytes_(addressWidthBytes == 1 ? 1U : 2U)
    , tenBitAddress_(tenBitAddress)
{
}

bool EepromDevice::isConfigured() const
{
    return static_cast<bool>(bus_);
}

std::expected<void, I2CError> EepromDevice::writeBytes(uint16_t offset, const I2CBytes& data) const
{
    if (!bus_) {
        CubeLog::error("EepromDevice has no I2C bus configured.");
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }

    I2CBytes txFrame = buildOffsetPrefix(offset);
    txFrame.insert(txFrame.end(), data.begin(), data.end());
    return bus_->write(address_, txFrame, tenBitAddress_);
}

std::expected<I2CBytes, I2CError> EepromDevice::readBytes(uint16_t offset, size_t length) const
{
    if (!bus_) {
        CubeLog::error("EepromDevice has no I2C bus configured.");
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }

    return bus_->writeRead(address_, buildOffsetPrefix(offset), length, tenBitAddress_);
}

I2CBytes EepromDevice::buildOffsetPrefix(uint16_t offset) const
{
    I2CBytes prefix;
    prefix.reserve(addressWidthBytes_);
    if (addressWidthBytes_ == 2) {
        prefix.push_back(static_cast<unsigned char>((offset >> 8) & 0xFF));
    }
    prefix.push_back(static_cast<unsigned char>(offset & 0xFF));
    return prefix;
}
