#include "../../src/api/eventDaemon/eventDaemonProtocol.h"
#include <gtest/gtest.h>

namespace {

TEST(EventDaemonProtocolTest, ParsesValidHelloFrame)
{
    const auto hello = EventDaemonProtocol::parseHelloFrame(
        R"({"type":"hello","appAuthId":"app-auth-123","sinceSeq":42,"sources":["interaction"]})");

    ASSERT_TRUE(hello.has_value());
    EXPECT_EQ(hello->appAuthId, "app-auth-123");
    EXPECT_EQ(hello->sinceSequence, 42u);
    ASSERT_TRUE(hello->sources.has_value());
    EXPECT_TRUE(hello->sources->contains("interaction"));
}

TEST(EventDaemonProtocolTest, RejectsMalformedHelloFrame)
{
    const auto hello = EventDaemonProtocol::parseHelloFrame("{not-json");

    ASSERT_FALSE(hello.has_value());
    EXPECT_EQ(hello.error().code, "invalid_request");
}

TEST(EventDaemonProtocolTest, RejectsMissingAppAuthId)
{
    const auto hello = EventDaemonProtocol::parseHelloFrame(R"({"type":"hello","sinceSeq":1})");

    ASSERT_FALSE(hello.has_value());
    EXPECT_EQ(hello.error().code, "invalid_request");
}

TEST(EventDaemonProtocolTest, RejectsWrongTypedTypeField)
{
    const auto hello = EventDaemonProtocol::parseHelloFrame(R"({"type":123,"appAuthId":"app-auth-123"})");

    ASSERT_FALSE(hello.has_value());
    EXPECT_EQ(hello.error().code, "invalid_request");
}

TEST(EventDaemonProtocolTest, SerializesHeartbeatAndErrorFrames)
{
    const auto heartbeat = EventDaemonProtocol::serializeHeartbeat(55);
    const auto error = EventDaemonProtocol::serializeError("resync_required", "queue overflow", 34);

    const auto heartbeatJson = nlohmann::json::parse(heartbeat);
    const auto errorJson = nlohmann::json::parse(error);

    EXPECT_EQ(heartbeatJson.value("type", ""), "heartbeat");
    EXPECT_EQ(heartbeatJson.value("nextSequence", 0u), 55u);

    EXPECT_EQ(errorJson.value("type", ""), "error");
    EXPECT_EQ(errorJson.value("code", ""), "resync_required");
    EXPECT_EQ(errorJson.value("nextSequence", 0u), 34u);
}

} // namespace
