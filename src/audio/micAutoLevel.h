/*
███╗   ███╗██╗ ██████╗ █████╗ ██╗   ██╗████████╗ ██████╗ ██╗     ███████╗██╗   ██╗███████╗██╗        ██╗  ██╗
████╗ ████║██║██╔════╝██╔══██╗██║   ██║╚══██╔══╝██╔═══██╗██║     ██╔════╝██║   ██║██╔════╝██║        ██║  ██║
██╔████╔██║██║██║     ███████║██║   ██║   ██║   ██║   ██║██║     █████╗  ██║   ██║█████╗  ██║        ███████║
██║╚██╔╝██║██║██║     ██╔══██║██║   ██║   ██║   ██║   ██║██║     ██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║        ██╔══██║
██║ ╚═╝ ██║██║╚██████╗██║  ██║╚██████╔╝   ██║   ╚██████╔╝███████╗███████╗ ╚████╔╝ ███████╗███████╗██╗██║  ██║
╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝    ╚═╝    ╚═════╝ ╚══════╝╚══════╝  ╚═══╝  ╚══════╝╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License
Copyright (c) 2026 A-McD Technology LLC
*/
#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct MicInputLevelHotWindow {
    int peak = 0;
    double clipRatio = 0.0;
};

struct MicInputLevelConfig {
    unsigned int sampleRate = 16000;
    unsigned int windowMs = 1000;
    int peakTrigger = 32000;
    int clipSampleThreshold = 32760;
    double clipRatioTrigger = 0.001;
    unsigned int consecutiveHotWindows = 2;
    unsigned int cooldownMs = 3000;
};

class MicInputLevelMonitor {
public:
    using Clock = std::chrono::steady_clock;
    using HotWindowCallback = std::function<void(const MicInputLevelHotWindow&)>;
    using TimeProvider = std::function<Clock::time_point()>;

    explicit MicInputLevelMonitor(
        MicInputLevelConfig config,
        HotWindowCallback callback,
        TimeProvider timeProvider = {});

    void observe(std::span<const int16_t> samples);

private:
    MicInputLevelConfig config_;
    HotWindowCallback callback_;
    TimeProvider timeProvider_;
    std::size_t samplesPerWindow_ = 1;
    std::size_t windowSampleCount_ = 0;
    std::size_t clippedSamples_ = 0;
    int peak_ = 0;
    unsigned int consecutiveHotCount_ = 0;
    std::optional<Clock::time_point> lastTrigger_;

    void analyzeSample(int16_t sample);
    void finalizeWindow();
};

struct MicSourceVolumeControllerConfig {
    int stepPercent = 5;
    int minPercent = 20;
};

struct CommandResult {
    int exitCode = -1;
    std::string output;
};

class ICommandRunner {
public:
    virtual ~ICommandRunner() = default;
    virtual CommandResult run(const std::vector<std::string>& command) = 0;
};

class SystemCommandRunner final : public ICommandRunner {
public:
    CommandResult run(const std::vector<std::string>& command) override;
};

class MicSourceVolumeController {
public:
    explicit MicSourceVolumeController(
        MicSourceVolumeControllerConfig config = {},
        std::shared_ptr<ICommandRunner> runner = std::make_shared<SystemCommandRunner>());

    bool initialize();
    bool lowerStep();
    bool restoreBaseline();
    std::optional<int> currentVolumePercent() const;

private:
    enum class Backend {
        None,
        Pactl,
        Wpctl
    };

    MicSourceVolumeControllerConfig config_;
    std::shared_ptr<ICommandRunner> runner_;
    Backend backend_ = Backend::None;
    std::string sourceId_;
    int baselineVolumePercent_ = -1;
    int currentVolumePercent_ = -1;
    bool initialized_ = false;
    bool autoAdjustEnabled_ = false;
    bool failureLogged_ = false;

    bool initializeWithPactl();
    bool initializeWithWpctl();
    bool setVolumePercent(int percent);
    std::optional<int> readPactlVolumePercent(const std::string& sourceId) const;
    std::optional<int> readWpctlVolumePercent() const;
    void disableAutoAdjust(const std::string& reason);

    static std::string trim(std::string value);
    static std::optional<int> extractFirstPercent(const std::string& text);
    static std::optional<double> extractFirstFloat(const std::string& text);
};
