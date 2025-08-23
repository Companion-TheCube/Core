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

// TODO: This needs a bit of a rewrite so that it is more easily used by NFC, FANCTRL, and other I2C related classes.

// Define i2c_msg if not included from <linux/i2c-dev.h>
#ifndef I2C_MSG_DEFINED
#define I2C_MSG_DEFINED
struct i2c_msg {
    __u16 addr;    // slave address
    __u16 flags;
    __u16 len;     // msg length
    __u8* buf;     // pointer to msg data
};
#endif

// Define I2C_M_TEN and I2C_M_RD if not included from <linux/i2c.h>
#ifndef I2C_M_TEN
#define I2C_M_TEN 0x0010
#endif
#ifndef I2C_M_RD
#define I2C_M_RD 0x0001
#endif


using base64String = std::string;

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
    I2C();
    ~I2C();

    // Register a logical handle for a target device address
    // addr is 7-bit by default; set tenbit=true for 10-bit addressing.
    // std::expected<int, I2CError> registerHandle(const std::string& handle, uint16_t addr, bool tenbit = false);
    std::expected<unsigned int, I2CError> registerHandle(const std::string& handle, uint16_t addr, bool tenbit = false);
    // Same signature style as your SPI class
    std::optional<base64String> transferTx(const std::string& handle, const base64String& txBase64);
    std::optional<base64String> transferTxRx(const std::string& handle, const base64String& txBase64, size_t rxLen);
    std::optional<base64String> transferTx(const unsigned int handle, const base64String& txBase64);
    std::optional<base64String> transferTxRx(const unsigned int handle, const base64String& txBase64, size_t rxLen);
    nlohmann::json getSettings(const std::string& handle);
    std::vector<std::string> getRegisteredHandles();
    bool setI2cDevicePath(const std::string& path);

private:
    std::mutex mutex_;
    std::string i2cDevicePath_;
    std::unordered_map<std::string, nlohmann::json> handles_;

    bool isHandleRegistered(const std::string& handle) const;
    nlohmann::json getHandleSettings(const std::string& handle);
    // Bind address, set 10-bit if needed
    bool bindAddress(int fd, uint16_t addr, bool tenbit);
    // write-only transaction (STOP after)
    std::optional<base64String> writeOnly(const unsigned int handle, const base64String& txBase64);
    // combined repeated-start write→read in one I2C_RDWR
    std::optional<base64String> writeRead(const unsigned int handle, const base64String& txBase64, size_t rxLen);
    static unsigned int nextHandleIndex;
};
