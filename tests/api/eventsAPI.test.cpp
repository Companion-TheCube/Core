#include "../../src/api/apiEventBroker.h"
#include "../../src/api/eventsAPI.h"
#include <gtest/gtest.h>

namespace {

TEST(EventsApiTest, WaitEndpointReturnsFilteredEvents)
{
    auto apiOwner = std::make_shared<API>();
    auto broker = apiOwner->getEventBroker();
    broker->registerSource("interaction");
    broker->registerSource("other");
    broker->publish("other", "ignored", nlohmann::json::object(), 900);
    broker->publish(
        "interaction",
        "tap",
        nlohmann::json { { "liftStateAfter", "unknown" }, { "sample", nullptr } },
        1000);

    EventsAPI api(apiOwner);
    const auto endpoints = api.getHttpEndpointData();
    ASSERT_EQ(endpoints.size(), 1u);

    httplib::Request req;
    req.params.emplace("sources", "interaction");
    req.params.emplace("since_seq", "0");
    req.params.emplace("limit", "10");
    req.params.emplace("wait_ms", "0");

    httplib::Response res;
    const auto error = std::get<1>(endpoints[0])(req, res);

    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_EQ(res.status, 200);

    const auto body = nlohmann::json::parse(res.body);
    ASSERT_EQ(body["events"].size(), 1u);
    EXPECT_EQ(body["events"][0].value("source", ""), "interaction");
    EXPECT_EQ(body["events"][0].value("event", ""), "tap");
    EXPECT_EQ(body.value("nextSequence", 0u), 2u);
    EXPECT_FALSE(body.value("timedOut", true));
}

TEST(EventsApiTest, WaitEndpointRejectsUnknownSources)
{
    auto apiOwner = std::make_shared<API>();
    auto broker = apiOwner->getEventBroker();
    broker->registerSource("interaction");

    EventsAPI api(apiOwner);
    const auto endpoints = api.getHttpEndpointData();

    httplib::Request req;
    req.params.emplace("sources", "missing");
    httplib::Response res;

    const auto error = std::get<1>(endpoints[0])(req, res);

    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS);
    EXPECT_EQ(res.status, 400);
}

TEST(EventsApiTest, WaitEndpointTimesOutWhenNoEventsArrive)
{
    auto apiOwner = std::make_shared<API>();
    auto broker = apiOwner->getEventBroker();
    broker->registerSource("interaction");

    EventsAPI api(apiOwner);
    const auto endpoints = api.getHttpEndpointData();

    httplib::Request req;
    req.params.emplace("sources", "interaction");
    req.params.emplace("wait_ms", "5");
    httplib::Response res;

    const auto error = std::get<1>(endpoints[0])(req, res);

    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_EQ(res.status, 200);

    const auto body = nlohmann::json::parse(res.body);
    EXPECT_TRUE(body["events"].empty());
    EXPECT_TRUE(body.value("timedOut", false));
    EXPECT_EQ(body.value("nextSequence", 123u), 0u);
}

} // namespace
