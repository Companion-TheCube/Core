#include "../../src/hardware/pi_i2c.h"
#include "../../src/utils.h"
#include <gtest/gtest.h>

namespace {

class ScopedConfigValue {
public:
    ScopedConfigValue(const std::string& key, const std::string& value)
        : key_(key)
        , hadPreviousValue_(Config::has(key))
    {
        if (hadPreviousValue_) {
            previousValue_ = Config::get(key, "");
        }
        Config::set(key_, value);
    }

    ~ScopedConfigValue()
    {
        if (hadPreviousValue_) {
            Config::set(key_, previousValue_);
        } else {
            Config::erase(key_);
        }
    }

private:
    std::string key_;
    bool hadPreviousValue_ = false;
    std::string previousValue_;
};

TEST(PiI2CBusTest, DisabledByConfigBlocksWriteBeforeDeviceAccess)
{
    ScopedConfigValue hardwareI2cEnabled("HARDWARE_I2C_ENABLED", "0");

    PiI2CBus bus;
    const auto writeResult = bus.write(0x2A, I2CBytes { 0x01, 0x02 }, false);
    const auto writeReadResult = bus.writeRead(0x2A, I2CBytes { 0x01 }, 2, false);

    ASSERT_FALSE(writeResult.has_value());
    EXPECT_EQ(writeResult.error(), I2CError::DISABLED_BY_CONFIG);
    ASSERT_FALSE(writeReadResult.has_value());
    EXPECT_EQ(writeReadResult.error(), I2CError::DISABLED_BY_CONFIG);
}

TEST(PiI2CBusTest, EnabledBusStillRequiresDevicePath)
{
    ScopedConfigValue hardwareI2cEnabled("HARDWARE_I2C_ENABLED", "1");

    PiI2CBus bus;
    const auto result = bus.write(0x2A, I2CBytes { 0x01 }, false);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), I2CError::NOT_INITIALIZED);
}

} // namespace
