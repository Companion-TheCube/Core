/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "micAutoLevel.h"
#ifndef LOGGER_H
#include <logger.h>
#endif

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

MicInputLevelMonitor::Clock::time_point defaultNow()
{
    return MicInputLevelMonitor::Clock::now();
}

} // namespace

MicInputLevelMonitor::MicInputLevelMonitor(
    MicInputLevelConfig config,
    HotWindowCallback callback,
    TimeProvider timeProvider)
    : config_(std::move(config))
    , callback_(std::move(callback))
    , timeProvider_(timeProvider ? std::move(timeProvider) : TimeProvider(defaultNow))
{
    const auto windowSamples = static_cast<std::uint64_t>(config_.sampleRate) * config_.windowMs;
    samplesPerWindow_ = static_cast<std::size_t>(std::max<std::uint64_t>(1, (windowSamples + 999) / 1000));
    if (config_.consecutiveHotWindows == 0) config_.consecutiveHotWindows = 1;
}

void MicInputLevelMonitor::observe(std::span<const int16_t> samples)
{
    for (int16_t sample : samples) {
        analyzeSample(sample);
        if (++windowSampleCount_ >= samplesPerWindow_) {
            finalizeWindow();
            windowSampleCount_ = 0;
            clippedSamples_ = 0;
            peak_ = 0;
        }
    }
}

void MicInputLevelMonitor::analyzeSample(int16_t sample)
{
    const int magnitude = std::abs(static_cast<int>(sample));
    peak_ = std::max(peak_, magnitude);
    if (magnitude >= config_.clipSampleThreshold) {
        ++clippedSamples_;
    }
}

void MicInputLevelMonitor::finalizeWindow()
{
    if (windowSampleCount_ == 0) return;

    const double clipRatio = static_cast<double>(clippedSamples_) / static_cast<double>(windowSampleCount_);
    const bool isHot = peak_ >= config_.peakTrigger && clipRatio >= config_.clipRatioTrigger;

    if (!isHot) {
        consecutiveHotCount_ = 0;
        return;
    }

    if (consecutiveHotCount_ < config_.consecutiveHotWindows) {
        ++consecutiveHotCount_;
    }

    if (consecutiveHotCount_ < config_.consecutiveHotWindows) return;

    const auto now = timeProvider_();
    if (lastTrigger_.has_value()) {
        const auto cooldown = std::chrono::milliseconds(config_.cooldownMs);
        if (now - *lastTrigger_ < cooldown) return;
    }

    lastTrigger_ = now;
    if (callback_) {
        callback_(MicInputLevelHotWindow {
            .peak = peak_,
            .clipRatio = clipRatio,
        });
    }
}

CommandResult SystemCommandRunner::run(const std::vector<std::string>& command)
{
    if (command.empty()) return {};

    int pipeFds[2] = { -1, -1 };
    if (pipe(pipeFds) != 0) {
        return { errno, std::strerror(errno) };
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return { errno, std::strerror(errno) };
    }

    if (pid == 0) {
        close(pipeFds[0]);
        dup2(pipeFds[1], STDOUT_FILENO);
        dup2(pipeFds[1], STDERR_FILENO);
        close(pipeFds[1]);

        std::vector<char*> argv;
        argv.reserve(command.size() + 1);
        for (const auto& part : command) {
            argv.push_back(const_cast<char*>(part.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv.front(), argv.data());
        _exit(127);
    }

    close(pipeFds[1]);
    std::string output;
    char buffer[512];
    ssize_t count = 0;
    while ((count = read(pipeFds[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<std::size_t>(count));
    }
    close(pipeFds[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return { errno, output };
    }
    if (WIFEXITED(status)) {
        return { WEXITSTATUS(status), output };
    }
    return { -1, output };
}

MicSourceVolumeController::MicSourceVolumeController(
    MicSourceVolumeControllerConfig config,
    std::shared_ptr<ICommandRunner> runner)
    : config_(std::move(config))
    , runner_(runner ? std::move(runner) : std::make_shared<SystemCommandRunner>())
{
}

bool MicSourceVolumeController::initialize()
{
    initialized_ = false;
    autoAdjustEnabled_ = false;
    backend_ = Backend::None;
    sourceId_.clear();
    baselineVolumePercent_ = -1;
    currentVolumePercent_ = -1;
    failureLogged_ = false;

    if (initializeWithPactl() || initializeWithWpctl()) {
        initialized_ = true;
        autoAdjustEnabled_ = true;
        CubeLog::info(
            "Mic auto level: source='" + sourceId_ + "', baselineVolume=" +
            std::to_string(baselineVolumePercent_) + "%");
        return true;
    }

    disableAutoAdjust("failed to resolve or read the default input source volume");
    return false;
}

bool MicSourceVolumeController::lowerStep()
{
    if (!initialized_ || !autoAdjustEnabled_) return false;

    const int oldPercent = currentVolumePercent_;
    const int targetPercent = std::max(config_.minPercent, oldPercent - config_.stepPercent);
    if (targetPercent >= oldPercent) return true;

    if (!setVolumePercent(targetPercent)) {
        disableAutoAdjust("failed to lower input source volume");
        return false;
    }

    currentVolumePercent_ = targetPercent;
    CubeLog::info(
        "Mic auto level: lowered source volume from " + std::to_string(oldPercent) +
        "% to " + std::to_string(targetPercent) + "%");
    return true;
}

bool MicSourceVolumeController::restoreBaseline()
{
    if (!initialized_ || baselineVolumePercent_ < 0) return false;
    if (currentVolumePercent_ == baselineVolumePercent_) {
        CubeLog::info(
            "Mic auto level: restored source volume to baseline " +
            std::to_string(baselineVolumePercent_) + "%");
        return true;
    }
    if (!setVolumePercent(baselineVolumePercent_)) {
        CubeLog::warning("Mic auto level: failed to restore input source baseline volume");
        return false;
    }
    currentVolumePercent_ = baselineVolumePercent_;
    CubeLog::info(
        "Mic auto level: restored source volume to baseline " +
        std::to_string(baselineVolumePercent_) + "%");
    return true;
}

std::optional<int> MicSourceVolumeController::currentVolumePercent() const
{
    if (currentVolumePercent_ < 0) return std::nullopt;
    return currentVolumePercent_;
}

bool MicSourceVolumeController::initializeWithPactl()
{
    const auto defaultSource = runner_->run({ "pactl", "get-default-source" });
    if (defaultSource.exitCode != 0) return false;

    const std::string sourceId = trim(defaultSource.output);
    if (sourceId.empty()) return false;

    const auto volumePercent = readPactlVolumePercent(sourceId);
    if (!volumePercent.has_value()) return false;

    backend_ = Backend::Pactl;
    sourceId_ = sourceId;
    baselineVolumePercent_ = *volumePercent;
    currentVolumePercent_ = *volumePercent;
    return true;
}

bool MicSourceVolumeController::initializeWithWpctl()
{
    const auto volumePercent = readWpctlVolumePercent();
    if (!volumePercent.has_value()) return false;

    backend_ = Backend::Wpctl;
    sourceId_ = "@DEFAULT_AUDIO_SOURCE@";
    baselineVolumePercent_ = *volumePercent;
    currentVolumePercent_ = *volumePercent;
    return true;
}

bool MicSourceVolumeController::setVolumePercent(int percent)
{
    percent = std::clamp(percent, 0, 150);
    CommandResult result;
    if (backend_ == Backend::Pactl) {
        result = runner_->run({
            "pactl",
            "set-source-volume",
            sourceId_,
            std::to_string(percent) + "%"
        });
    } else if (backend_ == Backend::Wpctl) {
        std::ostringstream value;
        value.setf(std::ios::fixed);
        value.precision(2);
        value << (static_cast<double>(percent) / 100.0);
        result = runner_->run({
            "wpctl",
            "set-volume",
            sourceId_,
            value.str()
        });
    } else {
        return false;
    }
    return result.exitCode == 0;
}

std::optional<int> MicSourceVolumeController::readPactlVolumePercent(const std::string& sourceId) const
{
    const auto result = runner_->run({ "pactl", "get-source-volume", sourceId });
    if (result.exitCode != 0) return std::nullopt;
    return extractFirstPercent(result.output);
}

std::optional<int> MicSourceVolumeController::readWpctlVolumePercent() const
{
    const auto result = runner_->run({ "wpctl", "get-volume", "@DEFAULT_AUDIO_SOURCE@" });
    if (result.exitCode != 0) return std::nullopt;
    const auto parsed = extractFirstFloat(result.output);
    if (!parsed.has_value()) return std::nullopt;
    return static_cast<int>(std::lround(std::clamp(*parsed, 0.0, 1.5) * 100.0));
}

void MicSourceVolumeController::disableAutoAdjust(const std::string& reason)
{
    autoAdjustEnabled_ = false;
    if (failureLogged_) return;
    failureLogged_ = true;
    CubeLog::warning("Mic auto level disabled: " + reason);
}

std::string MicSourceVolumeController::trim(std::string value)
{
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && isSpace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

std::optional<int> MicSourceVolumeController::extractFirstPercent(const std::string& text)
{
    std::string digits;
    for (char ch : text) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            digits.push_back(ch);
            continue;
        }
        if (ch == '%' && !digits.empty()) {
            try {
                return std::stoi(digits);
            } catch (...) {
                return std::nullopt;
            }
        }
        digits.clear();
    }
    return std::nullopt;
}

std::optional<double> MicSourceVolumeController::extractFirstFloat(const std::string& text)
{
    std::string token;
    bool seenDigit = false;
    for (char ch : text) {
        const bool digit = std::isdigit(static_cast<unsigned char>(ch)) != 0;
        if (digit || (ch == '.' && !token.empty() && token.find('.') == std::string::npos)) {
            token.push_back(ch);
            seenDigit = seenDigit || digit;
            continue;
        }
        if (seenDigit) {
            try {
                return std::stod(token);
            } catch (...) {
                return std::nullopt;
            }
        }
        token.clear();
        seenDigit = false;
    }
    if (!seenDigit) return std::nullopt;
    try {
        return std::stod(token);
    } catch (...) {
        return std::nullopt;
    }
}
