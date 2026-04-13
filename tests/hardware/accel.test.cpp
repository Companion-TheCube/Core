#include "../../src/hardware/accel.h"
#include <gtest/gtest.h>

#include <deque>

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

        if (!queuedResponses.empty()) {
            auto response = queuedResponses.front();
            queuedResponses.pop_front();
            return response;
        }

        return readResponse;
    }

    std::vector<WriteCall> writeCalls;
    std::vector<WriteReadCall> writeReadCalls;
    std::optional<I2CError> writeError;
    std::optional<I2CError> writeReadError;
    I2CBytes readResponse;
    std::deque<I2CBytes> queuedResponses;
};

I2CBytes samplePayload(int16_t x, int16_t y, int16_t z)
{
    return I2CBytes {
        static_cast<unsigned char>(x & 0xFF),
        static_cast<unsigned char>((x >> 8) & 0xFF),
        static_cast<unsigned char>(y & 0xFF),
        static_cast<unsigned char>((y >> 8) & 0xFF),
        static_cast<unsigned char>(z & 0xFF),
        static_cast<unsigned char>((z >> 8) & 0xFF),
    };
}

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
    bus->queuedResponses.push_back(I2CBytes { 0x24 });
    bus->queuedResponses.push_back(samplePayload(8192, -8192, 4096));

    Bmi270Accelerometer accelerometer(bus);
    const auto sample = accelerometer.readAcceleration();

    ASSERT_TRUE(sample.has_value());
    EXPECT_FLOAT_EQ(sample->xG, 1.0f);
    EXPECT_FLOAT_EQ(sample->yG, -1.0f);
    EXPECT_FLOAT_EQ(sample->zG, 0.5f);
    ASSERT_EQ(bus->writeReadCalls.size(), 2u);
    EXPECT_EQ(bus->writeReadCalls[1].txData, (I2CBytes { 0x0C }));
    EXPECT_EQ(bus->writeReadCalls[1].rxLen, 6u);
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

TEST(Bmi270AccelerometerTest, PollInteractionStatusDetectsTapAndMotion)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->queuedResponses.push_back(I2CBytes { 0x24 });
    bus->queuedResponses.push_back(samplePayload(0, 0, 8192));
    bus->queuedResponses.push_back(samplePayload(0, 0, 16384));

    Bmi270Accelerometer accelerometer(bus);
    ASSERT_TRUE(accelerometer.configureInteractionDetection(Bmi270InteractionConfig {}).has_value());

    const auto first = accelerometer.pollInteractionStatus();
    ASSERT_TRUE(first.has_value());
    EXPECT_FALSE(first->interruptStatus.tapDetected);
    EXPECT_FALSE(first->interruptStatus.motionDetected);

    const auto second = accelerometer.pollInteractionStatus();
    ASSERT_TRUE(second.has_value());
    EXPECT_TRUE(second->interruptStatus.tapDetected);
    EXPECT_TRUE(second->interruptStatus.motionDetected);
    EXPECT_FALSE(second->interruptStatus.noMotionDetected);
}

TEST(Bmi270AccelerometerTest, PollInteractionStatusDetectsNoMotionForSmallChange)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->queuedResponses.push_back(I2CBytes { 0x24 });
    bus->queuedResponses.push_back(samplePayload(0, 0, 8192));
    bus->queuedResponses.push_back(samplePayload(100, -100, 8180));

    Bmi270Accelerometer accelerometer(bus);
    ASSERT_TRUE(accelerometer.configureInteractionDetection(Bmi270InteractionConfig {}).has_value());

    ASSERT_TRUE(accelerometer.pollInteractionStatus().has_value());
    const auto second = accelerometer.pollInteractionStatus();

    ASSERT_TRUE(second.has_value());
    EXPECT_FALSE(second->interruptStatus.tapDetected);
    EXPECT_FALSE(second->interruptStatus.motionDetected);
    EXPECT_TRUE(second->interruptStatus.noMotionDetected);
}

TEST(Bmi270AccelerometerTest, UnexpectedChipIdMarksDeviceUnavailable)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->readResponse = I2CBytes { 0x00 };

    Bmi270Accelerometer accelerometer(bus);
    const auto initResult = accelerometer.initialize();

    ASSERT_FALSE(initResult.has_value());
    EXPECT_EQ(initResult.error(), Bmi270Error::UnexpectedChipId);
    EXPECT_FALSE(accelerometer.isAvailable());
}

} // namespace
