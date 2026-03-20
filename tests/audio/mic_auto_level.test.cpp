#include "audio/micAutoLevel.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstdio>

namespace {

using Clock = MicInputLevelMonitor::Clock;

class FakeRunner final : public ICommandRunner {
public:
    CommandResult run(const std::vector<std::string>& command) override
    {
        calls.push_back(command);
        if (command.empty()) return {};

        if (command[0] == "pactl") {
            if (pactlUnavailable) return { 127, "missing" };
            if (command.size() >= 2 && command[1] == "get-default-source") {
                return { 0, pactlSource + "\n" };
            }
            if (command.size() >= 3 && command[1] == "get-source-volume") {
                return { 0, "Volume: front-left: 47104 / " + std::to_string(volumePercent) + "% / -7.50 dB\n" };
            }
            if (command.size() >= 4 && command[1] == "set-source-volume") {
                volumePercent = parsePercent(command[3], volumePercent);
                ++setCount;
                return { 0, "" };
            }
        }

        if (command[0] == "wpctl") {
            if (wpctlUnavailable) return { 127, "missing" };
            if (command.size() >= 2 && command[1] == "get-volume") {
                return { 0, "Volume: " + formatFraction(volumePercent) + "\n" };
            }
            if (command.size() >= 4 && command[1] == "set-volume") {
                volumePercent = static_cast<int>(std::lround(std::stod(command[3]) * 100.0));
                ++setCount;
                return { 0, "" };
            }
        }

        return { 1, "unsupported" };
    }

    static int parsePercent(const std::string& value, int fallback)
    {
        try {
            auto trimmed = value;
            if (!trimmed.empty() && trimmed.back() == '%') trimmed.pop_back();
            return std::stoi(trimmed);
        } catch (...) {
            return fallback;
        }
    }

    static std::string formatFraction(int percent)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(percent) / 100.0);
        return buf;
    }

    bool pactlUnavailable = false;
    bool wpctlUnavailable = false;
    int volumePercent = 75;
    int setCount = 0;
    std::string pactlSource = "alsa_input.usb-test.analog-stereo";
    std::vector<std::vector<std::string>> calls;
};

std::vector<int16_t> makeWindow(std::size_t count, int16_t sampleValue)
{
    return std::vector<int16_t>(count, sampleValue);
}

} // namespace

TEST(MicInputLevelMonitorTest, QuietSpeechDoesNotTrigger)
{
    MicInputLevelConfig config;
    config.sampleRate = 16000;
    config.windowMs = 1000;
    config.consecutiveHotWindows = 2;

    int triggerCount = 0;
    MicInputLevelMonitor monitor(config, [&](const MicInputLevelHotWindow&) { ++triggerCount; });

    monitor.observe(makeWindow(16000, 500));
    monitor.observe(makeWindow(16000, 600));

    EXPECT_EQ(triggerCount, 0);
}

TEST(MicInputLevelMonitorTest, LoudButUnclippedSpeechDoesNotTrigger)
{
    MicInputLevelConfig config;
    config.sampleRate = 16000;
    config.windowMs = 1000;
    config.consecutiveHotWindows = 2;

    int triggerCount = 0;
    MicInputLevelMonitor monitor(config, [&](const MicInputLevelHotWindow&) { ++triggerCount; });

    monitor.observe(makeWindow(16000, 32100));
    monitor.observe(makeWindow(16000, 32100));

    EXPECT_EQ(triggerCount, 0);
}

TEST(MicInputLevelMonitorTest, ClippedSignalTriggersAfterTwoHotWindows)
{
    MicInputLevelConfig config;
    config.sampleRate = 16000;
    config.windowMs = 1000;
    config.consecutiveHotWindows = 2;

    int triggerCount = 0;
    MicInputLevelHotWindow lastHotWindow;
    Clock::time_point now = Clock::now();
    MicInputLevelMonitor monitor(
        config,
        [&](const MicInputLevelHotWindow& hotWindow) {
            ++triggerCount;
            lastHotWindow = hotWindow;
        },
        [&]() { return now; });

    monitor.observe(makeWindow(16000, 32760));
    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));

    EXPECT_EQ(triggerCount, 1);
    EXPECT_GE(lastHotWindow.peak, 32760);
    EXPECT_GE(lastHotWindow.clipRatio, 0.001);
}

TEST(MicInputLevelMonitorTest, CooldownPreventsRapidRetriggering)
{
    MicInputLevelConfig config;
    config.sampleRate = 16000;
    config.windowMs = 1000;
    config.consecutiveHotWindows = 2;
    config.cooldownMs = 3000;

    int triggerCount = 0;
    Clock::time_point now = Clock::now();
    MicInputLevelMonitor monitor(
        config,
        [&](const MicInputLevelHotWindow&) { ++triggerCount; },
        [&]() { return now; });

    monitor.observe(makeWindow(16000, 32760));
    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    EXPECT_EQ(triggerCount, 1);

    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    EXPECT_EQ(triggerCount, 2);
}

TEST(MicSourceVolumeControllerTest, InitializesFromPactlDefaultSourceAndVolume)
{
    auto runner = std::make_shared<FakeRunner>();
    runner->volumePercent = 77;
    MicSourceVolumeController controller({}, runner);

    ASSERT_TRUE(controller.initialize());
    ASSERT_TRUE(controller.currentVolumePercent().has_value());
    EXPECT_EQ(*controller.currentVolumePercent(), 77);
}

TEST(MicSourceVolumeControllerTest, FallsBackToWpctlWhenPactlFails)
{
    auto runner = std::make_shared<FakeRunner>();
    runner->pactlUnavailable = true;
    runner->volumePercent = 42;
    MicSourceVolumeController controller({}, runner);

    ASSERT_TRUE(controller.initialize());
    ASSERT_TRUE(controller.currentVolumePercent().has_value());
    EXPECT_EQ(*controller.currentVolumePercent(), 42);
}

TEST(MicSourceVolumeControllerTest, RespectsMinimumFloorAndRestoresBaseline)
{
    auto runner = std::make_shared<FakeRunner>();
    runner->volumePercent = 22;

    MicSourceVolumeControllerConfig config;
    config.stepPercent = 5;
    config.minPercent = 20;
    MicSourceVolumeController controller(config, runner);

    ASSERT_TRUE(controller.initialize());
    EXPECT_TRUE(controller.lowerStep());
    ASSERT_TRUE(controller.currentVolumePercent().has_value());
    EXPECT_EQ(*controller.currentVolumePercent(), 20);
    EXPECT_EQ(runner->setCount, 1);

    EXPECT_TRUE(controller.lowerStep());
    EXPECT_EQ(runner->setCount, 1);

    EXPECT_TRUE(controller.restoreBaseline());
    ASSERT_TRUE(controller.currentVolumePercent().has_value());
    EXPECT_EQ(*controller.currentVolumePercent(), 22);
    EXPECT_EQ(runner->setCount, 2);
}

TEST(MicAutoLevelIntegrationTest, SustainedOverdriveCausesRepeatedStepDownAfterCooldown)
{
    MicInputLevelConfig config;
    config.sampleRate = 16000;
    config.windowMs = 1000;
    config.consecutiveHotWindows = 2;
    config.cooldownMs = 3000;

    Clock::time_point now = Clock::now();
    int adjustmentCount = 0;
    MicInputLevelMonitor monitor(
        config,
        [&](const MicInputLevelHotWindow&) { ++adjustmentCount; },
        [&]() { return now; });

    monitor.observe(makeWindow(16000, 32760));
    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    EXPECT_EQ(adjustmentCount, 1);

    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    EXPECT_EQ(adjustmentCount, 1);

    now += std::chrono::seconds(1);
    monitor.observe(makeWindow(16000, 32760));
    EXPECT_EQ(adjustmentCount, 2);
}

TEST(MicAutoLevelIntegrationTest, HotInputDoesNotBreakWhenControllerIsUnavailable)
{
    MicInputLevelConfig config;
    config.sampleRate = 16000;
    config.windowMs = 1000;
    config.consecutiveHotWindows = 2;

    auto runner = std::make_shared<FakeRunner>();
    runner->pactlUnavailable = true;
    runner->wpctlUnavailable = true;
    MicSourceVolumeController controller({}, runner);
    EXPECT_FALSE(controller.initialize());

    int triggerCount = 0;
    MicInputLevelMonitor monitor(config, [&](const MicInputLevelHotWindow&) {
        ++triggerCount;
        EXPECT_FALSE(controller.lowerStep());
    });

    EXPECT_NO_THROW({
        monitor.observe(makeWindow(16000, 32760));
        monitor.observe(makeWindow(16000, 32760));
    });
    EXPECT_EQ(triggerCount, 1);
}
