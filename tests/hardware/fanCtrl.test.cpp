#include "../../src/hardware/fanCtrl.h"
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

TEST(FanControllerTest, EmitsExpectedCommandFrames)
{
    auto bus = std::make_shared<FakeI2CBus>();
    FanController controller(bus, 0x2A, true);

    ASSERT_TRUE(controller.setEnabled(true).has_value());
    ASSERT_TRUE(controller.setControlMode(FanControlMode::ManualDuty).has_value());
    ASSERT_TRUE(controller.setManualDutyPercent(150).has_value());

    ASSERT_EQ(bus->writeCalls.size(), 3u);
    EXPECT_EQ(bus->writeCalls[0].address, 0x2A);
    EXPECT_TRUE(bus->writeCalls[0].tenBit);
    EXPECT_EQ(bus->writeCalls[0].txData, (I2CBytes { 0x01, 0x01 }));
    EXPECT_EQ(bus->writeCalls[1].txData, (I2CBytes { 0x02, 0x01 }));
    EXPECT_EQ(bus->writeCalls[2].txData, (I2CBytes { 0x03, 100 }));
}

TEST(FanControllerTest, ReadStatusParsesProtocolFlagsAndTach)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->readResponse = I2CBytes {
        0x07,
        0x07,
        0x55,
        0x34,
        0x12,
        0xA0
    };

    FanController controller(bus, 0x2A);
    const auto status = controller.readStatus();

    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(bus->writeReadCalls.size(), 1u);
    EXPECT_EQ(bus->writeReadCalls[0].txData, (I2CBytes { 0x00 }));
    EXPECT_EQ(bus->writeReadCalls[0].rxLen, 6u);
    EXPECT_TRUE(status->enabled);
    EXPECT_EQ(status->mode, FanControlMode::ManualDuty);
    EXPECT_EQ(status->targetDutyPercent, 85);
    ASSERT_TRUE(status->tachRpm.has_value());
    EXPECT_EQ(*status->tachRpm, 0x1234);
    EXPECT_EQ(status->faultFlags, 0xA0);
    EXPECT_EQ(status->protocolVersion, 0x07);
}

TEST(FanControllerTest, ReadStatusHandlesMissingTachAndShortReads)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->readResponse = I2CBytes {
        0x01,
        0x01,
        0x20,
        0x00,
        0x00,
        0x00
    };

    FanController controller(bus, 0x2A);
    auto status = controller.readStatus();

    ASSERT_TRUE(status.has_value());
    EXPECT_FALSE(status->tachRpm.has_value());
    EXPECT_TRUE(status->enabled);
    EXPECT_EQ(status->mode, FanControlMode::Disabled);

    bus->readResponse = I2CBytes { 0x01, 0x01, 0x20 };
    status = controller.readStatus();
    ASSERT_FALSE(status.has_value());
    EXPECT_EQ(status.error(), I2CError::IO_FAILED);
}

TEST(FanControllerTest, ReadProtocolVersionAndNullBusErrorsAreReported)
{
    auto bus = std::make_shared<FakeI2CBus>();
    bus->readResponse = I2CBytes { 0x09 };

    FanController configuredController(bus, 0x2A);
    const auto version = configuredController.readProtocolVersion();

    ASSERT_TRUE(version.has_value());
    EXPECT_EQ(*version, 0x09);

    FanController unconfiguredController(nullptr, 0x2A);
    const auto writeResult = unconfiguredController.setEnabled(true);
    const auto readResult = unconfiguredController.readStatus();

    ASSERT_FALSE(writeResult.has_value());
    EXPECT_EQ(writeResult.error(), I2CError::NOT_INITIALIZED);
    ASSERT_FALSE(readResult.has_value());
    EXPECT_EQ(readResult.error(), I2CError::NOT_INITIALIZED);
}

} // namespace
