/*
██████╗ ██╗        ██╗██████╗  ██████╗   ██╗  ██╗
██╔══██╗██║        ██║╚════██╗██╔════╝   ██║  ██║
██████╔╝██║        ██║ █████╔╝██║        ███████║
██╔═══╝ ██║        ██║██╔═══╝ ██║        ██╔══██║
██║     ██║███████╗██║███████╗╚██████╗██╗██║  ██║
╚═╝     ╚═╝╚══════╝╚═╝╚══════╝ ╚═════╝╚═╝╚═╝  ╚═╝
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
This file defines the local I2C abstraction used by internal hardware drivers.
ILocalI2CBus is the test seam for NFC, EEPROM, and fan-control logic.
PiI2CBus is the concrete Linux /dev/i2c-* implementation for devices wired directly to the Raspberry Pi.
*/

#pragma once
#ifndef PI_I2C_H
#define PI_I2C_H

#include <expected>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <mutex>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifndef LOGGER_H
#include <logger.h>
#endif

using I2CBytes = std::vector<unsigned char>;

enum class I2CError {
    INVALID_ADDRESS,
    INVALID_DEVICE_PATH,
    INVALID_ARGUMENT,
    NOT_INITIALIZED,
    OPEN_FAILED,
    IOCTL_FAILED,
    IO_FAILED
};

class ILocalI2CBus {
public:
    virtual ~ILocalI2CBus() = default;

    virtual std::expected<void, I2CError> write(uint16_t address, const I2CBytes& txData, bool tenBit = false) = 0;
    virtual std::expected<I2CBytes, I2CError> writeRead(
        uint16_t address,
        const I2CBytes& txData,
        size_t rxLen,
        bool tenBit = false) = 0;
};

class PiI2CBus final : public ILocalI2CBus {
public:
    explicit PiI2CBus(const std::string& devicePath = "");
    ~PiI2CBus();

    bool setI2cDevicePath(const std::string& path);
    const std::string& getI2cDevicePath() const;

    std::expected<void, I2CError> write(uint16_t address, const I2CBytes& txData, bool tenBit = false) override;
    std::expected<I2CBytes, I2CError> writeRead(
        uint16_t address,
        const I2CBytes& txData,
        size_t rxLen,
        bool tenBit = false) override;

private:
    bool validateAddress(uint16_t address, bool tenBit) const;
    bool bindAddress(int fd, uint16_t address, bool tenBit) const;
    int openDevice() const;

    mutable std::mutex mutex_;
    std::string i2cDevicePath_;
};

using I2C = PiI2CBus;

#endif
