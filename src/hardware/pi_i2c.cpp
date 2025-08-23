// I2C.hpp
#pragma once
#include <cppcodec/base64_rfc4648.hpp>
#include <expected>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <utils.h>
#include <vector>
#ifndef LOGGER_H
#include <logger.h>
#endif

// TODO: Break the I2C class out into header and implementation files like SPI class.

enum class I2CError {
    INVALID_HANDLE,
    HANDLE_ALREADY_REGISTERED,
    INVALID_ADDRESS,
    INVALID_DEVICE_PATH,
    NOT_INITIALIZED,
    OPEN_FAILED,
    IOCTL_FAILED,
    IO_FAILED
};

class I2C {
public:
    I2C() { CubeLog::info("I2C class initialized"); }
    ~I2C() { CubeLog::info("I2C class destroyed"); }

    // Register a logical handle for a target device address
    // addr is 7-bit by default; set tenbit=true for 10-bit addressing.
    std::expected<int, I2CError> registerHandle(const std::string& handle,
        uint16_t addr,
        bool tenbit = false)
    {
        // TODO: move the sanitizeHandleString function to utils.h/cpp
        if (!sanitizeHandleString(handle)) {
            CubeLog::error("Invalid I2C handle name: " + handle);
            return std::unexpected(I2CError::INVALID_HANDLE);
        }
        // 7-bit: 0..0x7F; 10-bit: 0..0x3FF
        if ((!tenbit && addr > 0x7F) || (tenbit && addr > 0x3FF)) {
            CubeLog::error("Invalid I2C address: " + std::to_string(addr));
            return std::unexpected(I2CError::INVALID_ADDRESS);
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (handles_.count(handle)) {
            CubeLog::error("I2C handle already registered: " + handle);
            return std::unexpected(I2CError::HANDLE_ALREADY_REGISTERED);
        }
        if (i2cDevicePath_.empty()) {
            CubeLog::error("I2C device path is not set.");
            return std::unexpected(I2CError::NOT_INITIALIZED);
        }

        nlohmann::json settings;
        settings["addr"] = addr;
        settings["tenbit"] = tenbit;
        handles_[handle] = settings;

        CubeLog::info("I2C handle registered: " + handle);
        return 0;
    }

    // Same signature style as your SPI class
    std::optional<base64String> transferTx(const std::string& handle,
        const base64String& txBase64)
    {
        return writeOnly(handle, txBase64);
    }

    std::optional<base64String> transferTxRx(const std::string& handle,
        const base64String& txBase64,
        size_t rxLen)
    {
        return writeRead(handle, txBase64, rxLen);
    }

    nlohmann::json getSettings(const std::string& handle)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return getHandleSettings(handle);
    }

    std::vector<std::string> getRegisteredHandles()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> out;
        out.reserve(handles_.size());
        for (auto& kv : handles_)
            out.push_back(kv.first);
        return out;
    }

    bool setI2cDevicePath(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        struct stat st {};
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

private:
    std::mutex mutex_;
    std::string i2cDevicePath_;
    std::unordered_map<std::string, nlohmann::json> handles_;

    bool isHandleRegistered(const std::string& handle) const
    {
        return handles_.find(handle) != handles_.end();
    }

    nlohmann::json getHandleSettings(const std::string& handle)
    {
        auto it = handles_.find(handle);
        if (it != handles_.end())
            return it->second;
        CubeLog::error("I2C handle not found: " + handle);
        return {};
    }

    // Bind address, set 10-bit if needed
    bool bindAddress(int fd, uint16_t addr, bool tenbit)
    {
        if (ioctl(fd, I2C_TENBIT, tenbit ? 1 : 0) < 0) {
            CubeLog::error("I2C_TENBIT failed");
            return false;
        }
        if (ioctl(fd, I2C_SLAVE, addr) < 0) {
            CubeLog::error("I2C_SLAVE failed");
            return false;
        }
        return true;
    }

    // write-only transaction (STOP after)
    std::optional<base64String> writeOnly(const std::string& handle,
        const base64String& txBase64)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isHandleRegistered(handle)) {
            CubeLog::error("I2C handle not registered: " + handle);
            return std::nullopt;
        }

        std::vector<unsigned char> tx;
        try {
            tx = cppcodec::base64_rfc4648::decode(txBase64);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("I2C base64 decode failed: ") + e.what());
            return std::nullopt;
        }

        int fd = open(i2cDevicePath_.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            CubeLog::error("Failed to open I2C device: " + i2cDevicePath_);
            return std::nullopt;
        }

        auto addr = static_cast<uint16_t>(handles_[handle].value("addr", 0));
        bool tenbit = handles_[handle].value("tenbit", false);

        if (!bindAddress(fd, addr, tenbit)) {
            close(fd);
            return std::nullopt;
        }

        ssize_t n = ::write(fd, tx.data(), tx.size());
        if (n < 0 || static_cast<size_t>(n) != tx.size()) {
            CubeLog::error("I2C write() failed");
            close(fd);
            return std::nullopt;
        }

        close(fd);
        // write-only returns empty rx payload
        return base64_encode_cube(std::vector<unsigned char> {});
    }

    // combined repeated-start write→read in one I2C_RDWR
    std::optional<base64String> writeRead(const std::string& handle,
        const base64String& txBase64,
        size_t rxLen)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isHandleRegistered(handle)) {
            CubeLog::error("I2C handle not registered: " + handle);
            return std::nullopt;
        }

        std::vector<unsigned char> wbuf;
        try {
            wbuf = cppcodec::base64_rfc4648::decode(txBase64);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("I2C base64 decode failed: ") + e.what());
            return std::nullopt;
        }

        std::vector<unsigned char> rbuf(rxLen, 0);

        int fd = open(i2cDevicePath_.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            CubeLog::error("Failed to open I2C device: " + i2cDevicePath_);
            return std::nullopt;
        }

        auto addr = static_cast<uint16_t>(handles_[handle].value("addr", 0));
        bool tenbit = handles_[handle].value("tenbit", false);

        if (!bindAddress(fd, addr, tenbit)) {
            close(fd);
            return std::nullopt;
        }

        // Build transaction
        struct i2c_rdwr_ioctl_data rdwr {};
        std::vector<i2c_msg> msgs((wbuf.empty() ? 0 : 1) + (rxLen ? 1 : 0));

        int idx = 0;
        if (!wbuf.empty()) {
            msgs[idx].addr = addr;
            msgs[idx].flags = tenbit ? I2C_M_TEN : 0;
            msgs[idx].len = static_cast<__u16>(wbuf.size());
            msgs[idx].buf = reinterpret_cast<__u8*>(wbuf.data());
            idx++;
        }
        if (rxLen) {
            msgs[idx].addr = addr;
            msgs[idx].flags = (tenbit ? I2C_M_TEN : 0) | I2C_M_RD;
            msgs[idx].len = static_cast<__u16>(rxLen);
            msgs[idx].buf = reinterpret_cast<__u8*>(rbuf.data());
            idx++;
        }

        rdwr.msgs = msgs.data();
        rdwr.nmsgs = static_cast<__u32>(msgs.size());

        if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
            CubeLog::error("ioctl(I2C_RDWR) failed");
            close(fd);
            return std::nullopt;
        }

        close(fd);
        return base64_encode_cube(rbuf);
    }
};
