/*
██████╗ ██╗        ██╗██████╗  ██████╗    ██████╗██████╗ ██████╗
██╔══██╗██║        ██║╚════██╗██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██████╔╝██║        ██║ █████╔╝██║        ██║     ██████╔╝██████╔╝
██╔═══╝ ██║        ██║██╔═══╝ ██║        ██║     ██╔═══╝ ██╔═══╝
██║     ██║███████╗██║███████╗╚██████╗██╗╚██████╗██║     ██║
╚═╝     ╚═╝╚══════╝╚═╝╚══════╝ ╚═════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
This file implements the local I2C abstraction used by internal Raspberry Pi hardware drivers.
PiI2CBus performs Linux I2C transactions against /dev/i2c-* and is intended to be injected behind ILocalI2CBus for tests.
*/

#include "pi_i2c.h"

PiI2CBus::PiI2CBus(const std::string& devicePath)
    : i2cDevicePath_(devicePath)
{
    CubeLog::info("PiI2CBus initialized");
}

PiI2CBus::~PiI2CBus()
{
    CubeLog::info("PiI2CBus destroyed");
}

bool PiI2CBus::setI2cDevicePath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    struct stat st { };
    if (stat(path.c_str(), &st) != 0) {
        CubeLog::error("I2C device path does not exist: " + path);
        return false;
    }
    if (!S_ISCHR(st.st_mode)) {
        CubeLog::error("I2C device path is not a character device: " + path);
        return false;
    }
    i2cDevicePath_ = path;
    CubeLog::info("I2C device path set to: " + path);
    return true;
}

const std::string& PiI2CBus::getI2cDevicePath() const
{
    return i2cDevicePath_;
}

std::expected<void, I2CError> PiI2CBus::write(uint16_t address, const I2CBytes& txData, bool tenBit)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validateAddress(address, tenBit)) {
        CubeLog::error("Invalid I2C address: " + std::to_string(address));
        return std::unexpected(I2CError::INVALID_ADDRESS);
    }
    if (i2cDevicePath_.empty()) {
        CubeLog::error("I2C device path is not set.");
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }
    if (txData.empty()) {
        return { };
    }

    const int fd = openDevice();
    if (fd < 0) {
        CubeLog::error("Failed to open I2C device: " + i2cDevicePath_);
        return std::unexpected(I2CError::OPEN_FAILED);
    }

    if (!bindAddress(fd, address, tenBit)) {
        close(fd);
        return std::unexpected(I2CError::IOCTL_FAILED);
    }

    const ssize_t bytesWritten = ::write(fd, txData.data(), txData.size());
    close(fd);

    if (bytesWritten < 0 || static_cast<size_t>(bytesWritten) != txData.size()) {
        CubeLog::error("I2C write() failed");
        return std::unexpected(I2CError::IO_FAILED);
    }

    return { };
}

std::expected<I2CBytes, I2CError> PiI2CBus::writeRead(
    uint16_t address,
    const I2CBytes& txData,
    size_t rxLen,
    bool tenBit)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validateAddress(address, tenBit)) {
        CubeLog::error("Invalid I2C address: " + std::to_string(address));
        return std::unexpected(I2CError::INVALID_ADDRESS);
    }
    if (i2cDevicePath_.empty()) {
        CubeLog::error("I2C device path is not set.");
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }

    I2CBytes rxData(rxLen, 0);

    const int fd = openDevice();
    if (fd < 0) {
        CubeLog::error("Failed to open I2C device: " + i2cDevicePath_);
        return std::unexpected(I2CError::OPEN_FAILED);
    }

    if (!bindAddress(fd, address, tenBit)) {
        close(fd);
        return std::unexpected(I2CError::IOCTL_FAILED);
    }

    struct i2c_rdwr_ioctl_data rdwr { };
    std::vector<i2c_msg> messages((txData.empty() ? 0 : 1) + (rxLen > 0 ? 1 : 0));

    size_t messageIndex = 0;
    if (!txData.empty()) {
        messages[messageIndex].addr = address;
        messages[messageIndex].flags = tenBit ? I2C_M_TEN : 0;
        messages[messageIndex].len = static_cast<__u16>(txData.size());
        messages[messageIndex].buf = const_cast<__u8*>(reinterpret_cast<const __u8*>(txData.data()));
        messageIndex++;
    }
    if (rxLen > 0) {
        messages[messageIndex].addr = address;
        messages[messageIndex].flags = (tenBit ? I2C_M_TEN : 0) | I2C_M_RD;
        messages[messageIndex].len = static_cast<__u16>(rxLen);
        messages[messageIndex].buf = reinterpret_cast<__u8*>(rxData.data());
        messageIndex++;
    }

    rdwr.msgs = messages.data();
    rdwr.nmsgs = static_cast<__u32>(messages.size());

    const int result = ioctl(fd, I2C_RDWR, &rdwr);
    close(fd);

    if (result < 0) {
        CubeLog::error("ioctl(I2C_RDWR) failed");
        return std::unexpected(I2CError::IOCTL_FAILED);
    }

    return rxData;
}

bool PiI2CBus::validateAddress(uint16_t address, bool tenBit) const
{
    return tenBit ? address <= 0x03FF : address <= 0x007F;
}

bool PiI2CBus::bindAddress(int fd, uint16_t address, bool tenBit) const
{
    if (ioctl(fd, I2C_TENBIT, tenBit ? 1 : 0) < 0) {
        CubeLog::error("I2C_TENBIT failed");
        return false;
    }
    if (ioctl(fd, I2C_SLAVE, address) < 0) {
        CubeLog::error("I2C_SLAVE failed");
        return false;
    }
    return true;
}

int PiI2CBus::openDevice() const
{
    return open(i2cDevicePath_.c_str(), O_RDWR | O_CLOEXEC);
}
