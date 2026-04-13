#include "../../src/hardware/accel.h"
#include <gtest/gtest.h>

namespace {

class FakeI2CBus final : public ILocalI2CBus {
public:
    struct WriteCall {
        uint16_t address = 0;
        I2CBytes txData;
        bool tenBit = false;
    };

    struct WriteReadCall {
        uint16_t address = 0;
        I2CBytes txData;
        size_t rxLen = 0;
        bool tenBit = false;
    };

    std::expected<void, I2CError> write(uint16_t address, const I2CBytes& txData, bool tenBit = false) override
    {
        writeCalls.push_back({
            .address = address,
            .txData = txData,
            .tenBit = tenBit,
        });

        if (writeError.has_value()) {
            return std::unexpected(*writeError);
        }

        return { };
    }

    std::expected<I2CBytes, I2CError> writeRead(
        uint16_t address,
        const I2CBytes& txData,
        size_t rxLen,
        bool tenBit = false) override
    {
        writeReadCalls.push_back({
            .address = address,
            .txData = txData,
            .rxLen = rxLen,
            .tenBit = tenBit,
        });

        if (writeReadError.has_value()) {
            return std::unexpected(*writeReadError);
        }

        return readResponse;
    }

    std::vector<WriteCall> writeCalls;
    std::vector<WriteReadCall> writeReadCalls;
    std::optional<I2CError> writeError;
    std::optional<I2CError> writeReadError;
    I2CBytes readResponse;
};

TEST(Bmi270AccelerometerTest, ReadsExpectedChipIdRegister)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->readResponse = I2CBytes { 0x24 };

    Bmi270Accelerometer accelerometer(bus);
    const auto chipId = accelerometer.readChipId();

    ASSERT_TRUE(chipId.has_value());
    EXPECT_EQ(*chipId, 0x24);
    ASSERT_EQ(bus->writeReadCalls.size(), 1u);
    EXPECT_EQ(bus->writeReadCalls[0].txData, (I2CBytes { 0x00 }));
    EXPECT_EQ(bus->writeReadCalls[0].rxLen, 1u);
}

TEST(Bmi270AccelerometerTest, DecodesAccelerationSamplesFromLittleEndianPayload)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->readResponse = I2CBytes {
        0x00, 0x40, // x = 16384 -> 1g
        0x00, 0xC0, // y = -16384 -> -1g
        0x00, 0x20, // z = 8192 -> 0.5g
    };

    Bmi270Accelerometer accelerometer(bus);
    const auto sample = accelerometer.readAcceleration();

    ASSERT_TRUE(sample.has_value());
    EXPECT_FLOAT_EQ(sample->xG, 1.0f);
    EXPECT_FLOAT_EQ(sample->yG, -1.0f);
    EXPECT_FLOAT_EQ(sample->zG, 0.5f);
    ASSERT_EQ(bus->writeReadCalls.size(), 1u);
    EXPECT_EQ(bus->writeReadCalls[0].txData, (I2CBytes { 0x0C }));
    EXPECT_EQ(bus->writeReadCalls[0].rxLen, 6u);
}

TEST(Bmi270AccelerometerTest, GestureConfigurationIsExplicitlyStubbed)
{
    auto bus = std::make_shared<FakeI2CBus>();
    Bmi270Accelerometer accelerometer(bus);

    Bmi270InteractionConfig config;
    config.tapDetectionEnabled = true;
    config.liftDetectionEnabled = true;

    const auto result = accelerometer.configureInteractionDetection(config);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Bmi270Error::NotImplemented);
}

TEST(Bmi270AccelerometerTest, InvalidGestureConfigurationIsRejected)
{
    auto bus = std::make_shared<FakeI2CBus>();
    Bmi270Accelerometer accelerometer(bus);

    Bmi270InteractionConfig config;
    config.tapPeakThresholdG = 0.0f;

    const auto result = accelerometer.configureInteractionDetection(config);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Bmi270Error::InvalidConfig);
}

} // namespace
