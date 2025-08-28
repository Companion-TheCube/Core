#include "pi_i2c.h"

// TODO: The I2C Class methods that take care of writing and reading to the I2C bus need to be rewritten so that
// they don't have to look up the handle by string name every time. Instead, we should be able to use the index directly.

unsigned int I2C::nextHandleIndex = 1; // Start from 1

I2C::I2C() { CubeLog::info("I2C class initialized"); }
I2C::~I2C() { CubeLog::info("I2C class destroyed"); }

// Register a logical handle for a target device address
// addr is 7-bit by default; set tenbit=true for 10-bit addressing.
std::expected<unsigned int, I2CError> I2C::registerHandle(const std::string& handle, uint16_t addr, bool tenbit)
{
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
    settings["index"] = I2C::nextHandleIndex;
    handles_[handle] = settings;

    CubeLog::info("I2C handle registered: " + handle);
    return I2C::nextHandleIndex++;
}

// Same signature style as your SPI class
std::optional<base64String> I2C::transferTx(const unsigned int handle, const base64String& txBase64)
{
    return writeOnly(handle, txBase64);
}

std::optional<base64String> I2C::transferTxRx(const unsigned int handle, const base64String& txBase64, size_t rxLen)
{
    return writeRead(handle, txBase64, rxLen);
}

std::optional<base64String> I2C::transferTx(const std::string& handle, const base64String& txBase64)
{
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json settings = getHandleSettings(handle);
    if (settings.is_null()) {
        return std::nullopt;
    }
    unsigned int index = settings.value("index", 0);
    if (index == 0) {
        CubeLog::error("I2C handle has no valid index: " + handle);
        return std::nullopt;
    }
    return writeOnly(index, txBase64);
}

std::optional<base64String> I2C::transferTxRx(const std::string& handle, const base64String& txBase64, size_t rxLen)
{
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json settings = getHandleSettings(handle);
    if (settings.is_null()) {
        return std::nullopt;
    }
    unsigned int index = settings.value("index", 0);
    if (index == 0) {
        CubeLog::error("I2C handle has no valid index: " + handle);
        return std::nullopt;
    }
    return writeRead(index, txBase64, rxLen);
}

nlohmann::json I2C::getSettings(const std::string& handle)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return getHandleSettings(handle);
}

std::vector<std::string> I2C::getRegisteredHandles()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    out.reserve(handles_.size());
    for (auto& kv : handles_)
        out.push_back(kv.first);
    return out;
}

bool I2C::setI2cDevicePath(const std::string& path)
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

bool I2C::isHandleRegistered(const std::string& handle) const
{
    return handles_.find(handle) != handles_.end();
}

nlohmann::json I2C::getHandleSettings(const std::string& handle)
{
    auto it = handles_.find(handle);
    if (it != handles_.end())
        return it->second;
    CubeLog::error("I2C handle not found: " + handle);
    return {};
}

// Bind address, set 10-bit if needed
bool I2C::bindAddress(int fd, uint16_t addr, bool tenbit)
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
std::optional<base64String> I2C::writeOnly(const unsigned int handle, const base64String& txBase64)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Find handle by index
    std::string handle_str;
    for (const auto& kv : handles_) {
        if (kv.second.value("index", 0) == handle) {
            handle_str = kv.first;
            break;
        }
    }

    if (handle_str.empty()) {
        CubeLog::error("I2C handle not registered for index: " + std::to_string(handle));
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

    auto addr = static_cast<uint16_t>(handles_[handle_str].value("addr", 0));
    bool tenbit = handles_[handle_str].value("tenbit", false);

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
std::optional<base64String> I2C::writeRead(const unsigned int handle, const base64String& txBase64, size_t rxLen)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find handle by index
    std::string handle_str;
    for (const auto& kv : handles_) {
        if (kv.second.value("index", 0) == handle) {
            handle_str = kv.first;
            break;
        }
    }

    if (handle_str.empty()) {
        CubeLog::error("I2C handle not registered for index: " + std::to_string(handle));
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

    auto addr = static_cast<uint16_t>(handles_[handle_str].value("addr", 0));
    bool tenbit = handles_[handle_str].value("tenbit", false);

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
