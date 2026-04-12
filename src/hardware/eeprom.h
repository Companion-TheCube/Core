/*
███████╗███████╗██████╗ ██████╗  ██████╗ ███╗   ███╗   ██╗  ██╗
██╔════╝██╔════╝██╔══██╗██╔══██╗██╔═══██╗████╗ ████║   ██║  ██║
█████╗  █████╗  ██████╔╝██████╔╝██║   ██║██╔████╔██║   ███████║
██╔══╝  ██╔══╝  ██╔═══╝ ██╔══██╗██║   ██║██║╚██╔╝██║   ██╔══██║
███████╗███████╗██║     ██║  ██║╚██████╔╝██║ ╚═╝ ██║██╗██║  ██║
╚══════╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚═╝╚═╝  ╚═╝
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
This file defines the EEPROM driver boundary for hardware wired directly to the Raspberry Pi.
EepromDevice depends on ILocalI2CBus so storage behavior can be exercised in tests without the EEPROM present.
*/

#pragma once
#ifndef EEPROM_H
#define EEPROM_H

#include "pi_i2c.h"

#include <expected>
#include <memory>

class EepromDevice {
public:
    EepromDevice(
        std::shared_ptr<ILocalI2CBus> bus,
        uint16_t address,
        size_t addressWidthBytes = 2,
        bool tenBitAddress = false);

    bool isConfigured() const;
    std::expected<void, I2CError> writeBytes(uint16_t offset, const I2CBytes& data) const;
    std::expected<I2CBytes, I2CError> readBytes(uint16_t offset, size_t length) const;

private:
    I2CBytes buildOffsetPrefix(uint16_t offset) const;

    std::shared_ptr<ILocalI2CBus> bus_;
    uint16_t address_ = 0;
    size_t addressWidthBytes_ = 2;
    bool tenBitAddress_ = false;
};

#endif
