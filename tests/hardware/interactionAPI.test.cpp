#include "../../src/hardware/interactionAPI.h"
#include "../../src/settings/globalSettings.h"
#include <gtest/gtest.h>

#include <deque>

namespace {

using Clock = std::chrono::steady_clock;

class FakeAccelerometer final : public Bmi270Accelerometer {
public:
    FakeAccelerometer()
        : Bmi270Accelerometer(nullptr, 0x68)
    {
    }

    bool isConfigured() const override { return true; }
    bool isInitialized() const override { return initialized; }
    bool isAvailable() const override { return true; }

    std::expected<void, Bmi270Error> initialize() override
    {
        initialized = true;
        return { };
    }

    std::expected<void, Bmi270Error> configureInteractionDetection(const Bmi270InteractionConfig& config) override
    {
        (void)config;
        return { };
    }

    std::expected<Bmi270InteractionStatus, Bmi270Error> pollInteractionStatus() override
    {
        if (!queuedStatuses.empty()) {
            latestStatus = queuedStatuses.front();
            queuedStatuses.pop_front();
        }
        return latestStatus;
    }

    mutable bool initialized = false;
    mutable std::deque<Bmi270InteractionStatus> queuedStatuses;
    mutable Bmi270InteractionStatus latestStatus;
};

class TestPeripheralManager : public PeripheralManager {
public:
    using PeripheralManager::runInteractionControlIteration;

    TestPeripheralManager(
        std::unique_ptr<Bmi270Accelerometer> accelerometerOverride,
        MonotonicNowReader monotonicNowReader,
        EpochMsReader epochMsReader)
        : PeripheralManager(
            nullptr,
            nullptr,
            std::move(accelerometerOverride),
            {},
            {},
            std::move(monotonicNowReader),
            std::move(epochMsReader),
            false,
            false,
            false)
    {
    }
};

Bmi270InteractionStatus interactionStatus(
    const Bmi270AccelerationSample& accelSample,
    bool tapDetected = false,
    bool motionDetected = false,
    bool noMotionDetected = false)
{
    Bmi270InteractionStatus status;
    status.latestSample = accelSample;
    status.interruptStatus.tapDetected = tapDetected;
    status.interruptStatus.motionDetected = motionDetected;
    status.interruptStatus.noMotionDetected = noMotionDetected;
    return status;
}

TEST(InteractionApiTest, StatusEndpointReturnsCurrentSnapshot)
{
    GlobalSettings defaults;

    auto accelerometer = std::make_unique<FakeAccelerometer>();
    accelerometer->queuedStatuses.push_back(interactionStatus({ 0.0f, 0.0f, 1.0f }, false, false, true));
    accelerometer->queuedStatuses.push_back(interactionStatus({ 0.7f, 0.0f, 1.2f }, true, true, false));

    auto nowMono = Clock::now();
    uint64_t nowEpochMs = 1000;
    auto manager = std::make_shared<TestPeripheralManager>(
        std::move(accelerometer),
        [&nowMono]() { return nowMono; },
        [&nowEpochMs]() { return nowEpochMs; });

    manager->runInteractionControlIteration();
    nowMono += std::chrono::milliseconds(200);
    nowEpochMs += 200;
    manager->runInteractionControlIteration();

    InteractionAPI api(manager);
    const auto endpoints = api.getHttpEndpointData();

    ASSERT_EQ(endpoints.size(), 2u);
    EXPECT_EQ(std::get<2>(endpoints[0]), "status");
    EXPECT_EQ(std::get<0>(endpoints[0]), PRIVATE_ENDPOINT | GET_ENDPOINT);

    httplib::Request req;
    httplib::Response res;
    const auto error = std::get<1>(endpoints[0])(req, res);

    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_EQ(res.status, 200);

    const auto body = nlohmann::json::parse(res.body);
    EXPECT_TRUE(body.value("available", false));
    EXPECT_TRUE(body.value("initialized", false));
    EXPECT_EQ(body.value("lastEventSequence", 0u), 1u);
    ASSERT_TRUE(body.contains("lastTapAtEpochMs"));
}

TEST(InteractionApiTest, EventsEndpointRespectsSinceSequenceAndLimit)
{
    GlobalSettings defaults;

    auto accelerometer = std::make_unique<FakeAccelerometer>();
    accelerometer->queuedStatuses.push_back(interactionStatus({ 0.0f, 0.0f, 1.0f }, false, false, true));
    accelerometer->queuedStatuses.push_back(interactionStatus({ 0.6f, 0.0f, 1.2f }, true, true, false));
    accelerometer->queuedStatuses.push_back(interactionStatus({ 0.7f, 0.0f, 1.2f }, true, true, false));

    auto nowMono = Clock::now();
    uint64_t nowEpochMs = 1000;
    auto manager = std::make_shared<TestPeripheralManager>(
        std::move(accelerometer),
        [&nowMono]() { return nowMono; },
        [&nowEpochMs]() { return nowEpochMs; });

    manager->runInteractionControlIteration();
    nowMono += std::chrono::milliseconds(200);
    nowEpochMs += 200;
    manager->runInteractionControlIteration();
    nowMono += std::chrono::milliseconds(200);
    nowEpochMs += 200;
    manager->runInteractionControlIteration();

    InteractionAPI api(manager);
    const auto endpoints = api.getHttpEndpointData();

    httplib::Request req;
    req.params.emplace("since_seq", "1");
    req.params.emplace("limit", "1");
    httplib::Response res;

    const auto error = std::get<1>(endpoints[1])(req, res);

    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_EQ(res.status, 200);

    const auto body = nlohmann::json::parse(res.body);
    ASSERT_TRUE(body.contains("events"));
    ASSERT_EQ(body["events"].size(), 1u);
    EXPECT_EQ(body["events"][0].value("sequence", 0u), 2u);
    EXPECT_EQ(body.value("nextSequence", 0u), 3u);
    EXPECT_FALSE(body.value("historyTruncated", true));
}

} // namespace
