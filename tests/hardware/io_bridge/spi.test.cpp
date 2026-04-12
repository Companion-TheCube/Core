#include "../../../src/hardware/io_bridge/ioBridge.h"
#include "../../../src/hardware/io_bridge/spi.h"
#include "../../../src/utils.h"
#include <gtest/gtest.h>

namespace {

class FakeIoBridgeTransport final : public IIoBridgeTransport {
public:
    std::expected<IoBridgePayload, IoBridgeError> transact(const IoBridgeTransaction& transaction) override
    {
        ++callCount;
        lastTransaction = transaction;
        return response;
    }

    int callCount = 0;
    IoBridgeTransaction lastTransaction;
    std::expected<IoBridgePayload, IoBridgeError> response = IoBridgePayload { 0x04, 0x05 };
};

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

TEST(BridgeSpiTest, DisabledByConfigPreventsBridgeTransfer)
{
    ScopedConfigValue hardwareSpiEnabled("HARDWARE_SPI_ENABLED", "0");

    auto transport = std::make_shared<FakeIoBridgeTransport>();
    auto session = std::make_shared<IoBridgeSession>(transport);
    SPI spi(session);

    ASSERT_TRUE(spi.registerHandle("flash", 1000000, 0).has_value());
    const auto result = spi.transferTxRx("flash", "AQID", 2);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(transport->callCount, 0);
}

TEST(BridgeSpiTest, EnabledByConfigAllowsBridgeTransfer)
{
    ScopedConfigValue hardwareSpiEnabled("HARDWARE_SPI_ENABLED", "1");

    auto transport = std::make_shared<FakeIoBridgeTransport>();
    auto session = std::make_shared<IoBridgeSession>(transport);
    SPI spi(session);

    ASSERT_TRUE(spi.registerHandle("flash", 1000000, 0).has_value());
    const auto result = spi.transferTxRx("flash", "AQID", 2);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(transport->callCount, 1);
    EXPECT_EQ(transport->lastTransaction.endpoint, IoBridgeEndpoint::Spi);
}

} // namespace
