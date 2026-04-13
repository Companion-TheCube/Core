/*
 ██████╗ ██╗      ██████╗ ██████╗  █████╗ ██╗     ███████╗███████╗████████╗████████╗██╗███╗   ██╗ ██████╗ ███████╗   ██╗  ██╗
██╔════╝ ██║     ██╔═══██╗██╔══██╗██╔══██╗██║     ██╔════╝██╔════╝╚══██╔══╝╚══██╔══╝██║████╗  ██║██╔════╝ ██╔════╝   ██║  ██║
██║  ███╗██║     ██║   ██║██████╔╝███████║██║     ███████╗█████╗     ██║      ██║   ██║██╔██╗ ██║██║  ███╗███████╗   ███████║
██║   ██║██║     ██║   ██║██╔══██╗██╔══██║██║     ╚════██║██╔══╝     ██║      ██║   ██║██║╚██╗██║██║   ██║╚════██║   ██╔══██║
╚██████╔╝███████╗╚██████╔╝██████╔╝██║  ██║███████╗███████║███████╗   ██║      ██║   ██║██║ ╚████║╚██████╔╝███████║██╗██║  ██║
 ╚═════╝ ╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝   ╚═╝      ╚═╝   ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#ifndef GLOBAL_SETTINGS_H
#define GLOBAL_SETTINGS_H
#ifndef UTILS_H
#include "utils.h"
#endif
#include <algorithm>
#include <filesystem>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include <mutex>
#include <nlohmann/json.hpp>
#include <vector>

struct GlobalSettings {
    enum class SettingType : int {
        LOG_VERBOSITY,
        SELECTED_FONT_PATH,
        LOG_LEVEL_PRINT,
        LOG_LEVEL_FILE,
        DEVELOPER_MODE_ENABLED,
        CPU_AND_MEMORY_DISPLAY_ENABLED,
        SSH_ENABLED,
        WIFI_ENABLED,
        WIFI_SSID,
        WIFI_PASSWORD,
        WIFI_STATIC_IP,
        WIFI_STATIC_SUBNET,
        WIFI_STATIC_GATEWAY,
        WIFI_STATIC_DNS1,
        WIFI_STATIC_DNS2,
        WIFI_STATIC_DNS,
        WIFI_STATIC_PROXY,
        WIFI_STATIC_PROXY_PORT,
        WIFI_STATIC_PROXY_USERNAME,
        WIFI_STATIC_PROXY_PASSWORD,
        WIFI_STATIC_PROXY_BYPASS,
        WIFI_STATIC_PROXY_PAC,
        WIFI_STATIC_PROXY_PAC_URL,
        BT_ENABLED,
        STATUS_IP_ADDRESS,
        STATUS_WIFI_MAC_ADDRESS,
        STATUS_BT_MAC_ADDRESS,
        STATUS_FCC_ID,
        IDLE_ANIMATION_ENABLED,
        IDLE_ANIMATION,
        SCREEN_BRIGHTNESS,
        SCREEN_AUTO_OFF,
        SCREEN_AUTO_OFF_TIME,
        NOTIFICATIONS_FROM_NETWORK_ENABLED,
        SYSTEM_VOLUME,
        NOTIFICATION_SOUND_VOLUME,
        ALARM_SOUND_VOLUME,
        ALARM_SNOOZE_MINUTES,
        VOICE_COMMAND_SOUND_VOLUME,
        NOTIFICATION_SOUND,
        ALARM_SOUND,
        VOICE_COMMAND_SOUND,
        GENERAL_AI_RESPONSE_MODE,
        NFC_ENABLED,
        PERSONALITY_ENABLED,
        MICROPHONE_ENABLED,
        PRESENCE_DETECTION_ENABLED,
        PRESENCE_ABSENT_TIMEOUT_SECS,
        EMOTION_CURIOSITY,
        EMOTION_PLAYFULNESS,
        EMOTION_EMPATHY,
        EMOTION_ASSERTIVENESS,
        EMOTION_ATTENTIVENESS,
        EMOTION_CAUTION,
        EMOTION_ANNOYANCE,

        MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS,
        MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS,
        MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS,
        MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS,
        FAN_CONTROL_ENABLED,
        FAN_CONTROL_POLL_INTERVAL_MS,
        FAN_CONTROL_HYSTERESIS_C,
        FAN_CONTROL_FAILSAFE_PERCENT,
        FAN_CONTROL_CURVE_POINTS,
        INTERACTION_DETECTION_ENABLED,
        INTERACTION_POLL_INTERVAL_MS,
        INTERACTION_EVENT_HISTORY_SIZE,
        INTERACTION_TAP_DEBOUNCE_MS,
        INTERACTION_LIFT_CONFIRM_MS,
        INTERACTION_REST_STABLE_MS,
        INTERACTION_LIFT_DELTA_THRESHOLD_MG,

        SETTING_TYPE_COUNT
    };

    /**
     * @brief Convert a SettingType to a string
     */
    static std::unordered_map<SettingType, std::string> settingTypeStringMap;

    /**
     * @brief Convert a string to a SettingType
     */
    static std::unordered_map<std::string, SettingType> stringSettingTypeMap;

    GlobalSettings()
    {
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        populateDefaultSettingsLocked();
    }

    static void setSettingCB(SettingType key, std::function<void()> callback)
    {
        settingChangeCallbacks.push_back({ key, callback });
    }

    static void setSettingCB(SettingType key, std::function<void(const nlohmann::json&)> callback)
    {
        settingChangeCallbacks.push_back({ key, [callback, key]() { callback(GlobalSettings::getSetting(key)); } });
    }

    static void callSettingCB(SettingType key)
    {
        CubeLog::debug("Calling setting change callback for " + std::to_string((int)key));
        for (auto& [settingKey, cb] : settingChangeCallbacks) {
            if (settingKey == key) {
                cb();
            }
        }
    }

    static bool setSetting(SettingType key, nlohmann::json::value_type value)
    {
        ensureInitialized();
        switch (key) {
        case SettingType::MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS:
        case SettingType::MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS:
        case SettingType::MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS:
        case SettingType::MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS: {
            int clampedValue = 10;
            if (value.is_number()) {
                clampedValue = std::clamp(value.get<int>(), 1, 30);
            }
            value = clampedValue;
            break;
        }
        case SettingType::PRESENCE_ABSENT_TIMEOUT_SECS: {
            int clampedValue = 15;
            if (value.is_number()) {
                clampedValue = std::clamp(value.get<int>(), 1, 120);
            }
            value = clampedValue;
            break;
        }
        case SettingType::FAN_CONTROL_POLL_INTERVAL_MS: {
            int clampedValue = 2000;
            if (value.is_number()) {
                clampedValue = clampFanControlPollIntervalMs(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::FAN_CONTROL_HYSTERESIS_C: {
            double clampedValue = 2.0;
            if (value.is_number()) {
                clampedValue = clampFanControlHysteresisC(value.get<double>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::FAN_CONTROL_FAILSAFE_PERCENT: {
            int clampedValue = 100;
            if (value.is_number()) {
                clampedValue = clampFanControlFailsafePercent(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::FAN_CONTROL_CURVE_POINTS:
            value = normalizeFanControlCurvePoints(value);
            break;
        case SettingType::INTERACTION_POLL_INTERVAL_MS: {
            int clampedValue = 50;
            if (value.is_number()) {
                clampedValue = clampInteractionPollIntervalMs(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::INTERACTION_EVENT_HISTORY_SIZE: {
            int clampedValue = 128;
            if (value.is_number()) {
                clampedValue = clampInteractionEventHistorySize(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::INTERACTION_TAP_DEBOUNCE_MS: {
            int clampedValue = 120;
            if (value.is_number()) {
                clampedValue = clampInteractionTapDebounceMs(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::INTERACTION_LIFT_CONFIRM_MS: {
            int clampedValue = 150;
            if (value.is_number()) {
                clampedValue = clampInteractionLiftConfirmMs(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::INTERACTION_REST_STABLE_MS: {
            int clampedValue = 500;
            if (value.is_number()) {
                clampedValue = clampInteractionRestStableMs(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        case SettingType::INTERACTION_LIFT_DELTA_THRESHOLD_MG: {
            int clampedValue = 200;
            if (value.is_number()) {
                clampedValue = clampInteractionLiftDeltaThresholdMg(value.get<int>());
            }
            value = clampedValue;
            break;
        }
        default:
            break;
        }
        {
            std::unique_lock<std::mutex> lock(settingChangeMutex);
            settings[settingTypeStringMap[key]] = value;
        }
        callSettingCB(key);
        return true;
    }
    static nlohmann::json getSettings()
    {
        ensureInitialized();
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        return settings;
    }
    static nlohmann::json::value_type getSetting(SettingType key)
    {
        ensureInitialized();
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        return settings[settingTypeStringMap[key]];
    }
    template <typename T>
    static T getSettingOfType(SettingType key)
    {
        ensureInitialized();
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        T s;
        try {
            s = settings[settingTypeStringMap[key]];
        } catch (nlohmann::json::exception e) {
            CubeLog::error("Error getting setting of type " + settingTypeStringMap[key] + ": " + e.what());
            return T();
        }
        return s;
    }
    static std::vector<std::string> getSettingsNames()
    {
        std::vector<std::string> settingNames;
        for (auto it = settings.begin(); it != settings.end(); it++) {
            settingNames.push_back(it.key());
        }
        return settingNames;
    }

    static std::string toString()
    {
        std::string output = "";
        switch (GlobalSettings::getSettingOfType<Logger::LogVerbosity>(SettingType::LOG_VERBOSITY)) {
        case Logger::LogVerbosity::MINIMUM:
            output += "Log Verbosity: Message only\n";
            break;
        case Logger::LogVerbosity::TIMESTAMP:
            output += "Log Verbosity: Timestamp and message\n";
            break;
        case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL:
            output += "Log Verbosity: Timestamp, log level, and message\n";
            break;
        case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE:
            output += "Log Verbosity: Timestamp, log level, source file, and message\n";
            break;
        case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE:
            output += "Log Verbosity: Timestamp, log level, source file, line number, and message\n";
            break;
        case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION:
            output += "Log Verbosity: Timestamp, log level, source file, line number, function name, and message\n";
            break;
        case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS:
            output += "Log Verbosity: Timestamp, log level, source file, line number, function name, number of logs, and message\n";
            break;
        default:
            output += "Log Verbosity: UNKNOWN\n";
        }
        switch (GlobalSettings::getSettingOfType<Logger::LogLevel>(SettingType::LOG_LEVEL_PRINT)) {
        case Logger::LogLevel::LOGGER_MORE_INFO:
            output += "Log Level for printing: MORE INFO\n";
            break;
        case Logger::LogLevel::LOGGER_INFO:
            output += "Log Level for printing: INFO\n";
            break;
        case Logger::LogLevel::LOGGER_WARNING:
            output += "Log Level for printing: WARNING\n";
            break;
        case Logger::LogLevel::LOGGER_ERROR:
            output += "Log Level for printing: ERROR\n";
            break;
        case Logger::LogLevel::LOGGER_CRITICAL:
            output += "Log Level for printing: CRITICAL\n";
            break;
        case Logger::LogLevel::LOGGER_FATAL:
            output += "Log Level for printing: FATAL\n";
            break;
        case Logger::LogLevel::LOGGER_OFF:
            output += "Log Level for printing: OFF\n";
            break;
        case Logger::LogLevel::LOGGER_DEBUG:
            output += "Log Level for printing: DEBUG\n";
            break;
        case Logger::LogLevel::LOGGER_DEBUG_SILLY:
            output += "Log Level for printing: DEBUG SILLY\n";
            break;
        default:
            output += "Log Level for printing: UNKNOWN\n";
        }
        switch (GlobalSettings::getSettingOfType<Logger::LogLevel>(SettingType::LOG_LEVEL_FILE)) {
        case Logger::LogLevel::LOGGER_MORE_INFO:
            output += "Log Level for log file: MORE INFO\n";
            break;
        case Logger::LogLevel::LOGGER_INFO:
            output += "Log Level for log file: INFO\n";
            break;
        case Logger::LogLevel::LOGGER_WARNING:
            output += "Log Level for log file: WARNING\n";
            break;
        case Logger::LogLevel::LOGGER_ERROR:
            output += "Log Level for log file: ERROR\n";
            break;
        case Logger::LogLevel::LOGGER_CRITICAL:
            output += "Log Level for log file: CRITICAL\n";
            break;
        case Logger::LogLevel::LOGGER_FATAL:
            output += "Log Level for log file: FATAL\n";
            break;
        case Logger::LogLevel::LOGGER_OFF:
            output += "Log Level for log file: OFF\n";
            break;
        case Logger::LogLevel::LOGGER_DEBUG:
            output += "Log Level for log file: DEBUG\n";
            break;
        case Logger::LogLevel::LOGGER_DEBUG_SILLY:
            output += "Log Level for log file: DEBUG SILLY\n";
            break;
        default:
            output += "Log Level for log file: UNKNOWN\n";
        }
        auto fontpath = std::filesystem::path(GlobalSettings::getSettingOfType<std::string>(SettingType::SELECTED_FONT_PATH));
        output += "Selected Font: " + std::string(fontpath.filename().string()) + "\n";
        output += "Developer Mode Enabled: " + std::to_string(GlobalSettings::getSettingOfType<bool>(SettingType::DEVELOPER_MODE_ENABLED)) + "\n";
        return output;
    }

private:
    static std::vector<std::pair<SettingType, std::function<void()>>> settingChangeCallbacks;
    static std::mutex settingChangeMutex;
    static nlohmann::json settings;
    static bool settingsInitialized;

    static void ensureInitialized()
    {
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        if (!settingsInitialized) {
            populateDefaultSettingsLocked();
        }
    }

    static std::string findDefaultFontPath()
    {
        const std::array<std::filesystem::path, 4> fontRoots = {
            std::filesystem::path("fonts"),
            std::filesystem::current_path() / "fonts",
            std::filesystem::current_path() / ".." / "fonts",
            std::filesystem::current_path() / "build" / "bin" / "fonts"
        };

        for (const auto& root : fontRoots) {
            for (const auto& fontPath : loadFontPaths(root)) {
                if (fontPath.find("Roboto-Regular.ttf") != std::string::npos) {
                    return fontPath;
                }
            }
        }
        return {};
    }

    static void populateDefaultSettingsLocked()
    {
        settings = nlohmann::json::object();
        settingsInitialized = true;

        const std::string defaultFontPath = findDefaultFontPath();
        if (defaultFontPath.empty()) {
            CubeLog::warning("Default font not found while initializing GlobalSettings.");
            settings[settingTypeStringMap[SettingType::SELECTED_FONT_PATH]] = "";
        } else {
            settings[settingTypeStringMap[SettingType::SELECTED_FONT_PATH]] = defaultFontPath;
        }

        settings[settingTypeStringMap[SettingType::LOG_VERBOSITY]] = Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE;
        settings[settingTypeStringMap[SettingType::LOG_LEVEL_PRINT]] = Logger::LogLevel::LOGGER_INFO;
        settings[settingTypeStringMap[SettingType::LOG_LEVEL_FILE]] = Logger::LogLevel::LOGGER_INFO;
        settings[settingTypeStringMap[SettingType::DEVELOPER_MODE_ENABLED]] = false;
        settings[settingTypeStringMap[SettingType::CPU_AND_MEMORY_DISPLAY_ENABLED]] = false;
        settings[settingTypeStringMap[SettingType::SSH_ENABLED]] = false;
        settings[settingTypeStringMap[SettingType::WIFI_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::BT_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::IDLE_ANIMATION_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::SCREEN_BRIGHTNESS]] = 100;
        settings[settingTypeStringMap[SettingType::SCREEN_AUTO_OFF]] = true;
        settings[settingTypeStringMap[SettingType::SCREEN_AUTO_OFF_TIME]] = 5;
        settings[settingTypeStringMap[SettingType::NOTIFICATIONS_FROM_NETWORK_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::SYSTEM_VOLUME]] = 100;
        settings[settingTypeStringMap[SettingType::NOTIFICATION_SOUND_VOLUME]] = 100;
        settings[settingTypeStringMap[SettingType::ALARM_SOUND_VOLUME]] = 100;
        settings[settingTypeStringMap[SettingType::ALARM_SNOOZE_MINUTES]] = 10;
        settings[settingTypeStringMap[SettingType::VOICE_COMMAND_SOUND_VOLUME]] = 100;
        settings[settingTypeStringMap[SettingType::NOTIFICATION_SOUND]] = "default";
        settings[settingTypeStringMap[SettingType::ALARM_SOUND]] = "default";
        settings[settingTypeStringMap[SettingType::VOICE_COMMAND_SOUND]] = "default";
        settings[settingTypeStringMap[SettingType::GENERAL_AI_RESPONSE_MODE]] = "popupOnly";
        settings[settingTypeStringMap[SettingType::NFC_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::PERSONALITY_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::MICROPHONE_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::PRESENCE_DETECTION_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::PRESENCE_ABSENT_TIMEOUT_SECS]] = 15;
        settings[settingTypeStringMap[SettingType::EMOTION_CURIOSITY]] = 80;
        settings[settingTypeStringMap[SettingType::EMOTION_PLAYFULNESS]] = 70;
        settings[settingTypeStringMap[SettingType::EMOTION_EMPATHY]] = 85;
        settings[settingTypeStringMap[SettingType::EMOTION_ASSERTIVENESS]] = 60;
        settings[settingTypeStringMap[SettingType::EMOTION_ATTENTIVENESS]] = 90;
        settings[settingTypeStringMap[SettingType::EMOTION_CAUTION]] = 50;
        settings[settingTypeStringMap[SettingType::EMOTION_ANNOYANCE]] = 0;
        settings[settingTypeStringMap[SettingType::MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS]] = 10;
        settings[settingTypeStringMap[SettingType::MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS]] = 10;
        settings[settingTypeStringMap[SettingType::MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS]] = 10;
        settings[settingTypeStringMap[SettingType::MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS]] = 10;
        settings[settingTypeStringMap[SettingType::FAN_CONTROL_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::FAN_CONTROL_POLL_INTERVAL_MS]] = 2000;
        settings[settingTypeStringMap[SettingType::FAN_CONTROL_HYSTERESIS_C]] = 2.0;
        settings[settingTypeStringMap[SettingType::FAN_CONTROL_FAILSAFE_PERCENT]] = 100;
        settings[settingTypeStringMap[SettingType::FAN_CONTROL_CURVE_POINTS]] = defaultFanControlCurvePoints();
        settings[settingTypeStringMap[SettingType::INTERACTION_DETECTION_ENABLED]] = true;
        settings[settingTypeStringMap[SettingType::INTERACTION_POLL_INTERVAL_MS]] = 50;
        settings[settingTypeStringMap[SettingType::INTERACTION_EVENT_HISTORY_SIZE]] = 128;
        settings[settingTypeStringMap[SettingType::INTERACTION_TAP_DEBOUNCE_MS]] = 120;
        settings[settingTypeStringMap[SettingType::INTERACTION_LIFT_CONFIRM_MS]] = 150;
        settings[settingTypeStringMap[SettingType::INTERACTION_REST_STABLE_MS]] = 500;
        settings[settingTypeStringMap[SettingType::INTERACTION_LIFT_DELTA_THRESHOLD_MG]] = 200;
    }

    static int clampFanControlPollIntervalMs(int value)
    {
        return std::clamp(value, 250, 10000);
    }

    static double clampFanControlHysteresisC(double value)
    {
        return std::clamp(value, 0.0, 10.0);
    }

    static int clampFanControlFailsafePercent(int value)
    {
        return std::clamp(value, 0, 100);
    }

    static int clampInteractionPollIntervalMs(int value)
    {
        return std::clamp(value, 20, 500);
    }

    static int clampInteractionEventHistorySize(int value)
    {
        return std::clamp(value, 32, 1024);
    }

    static int clampInteractionTapDebounceMs(int value)
    {
        return std::clamp(value, 50, 1000);
    }

    static int clampInteractionLiftConfirmMs(int value)
    {
        return std::clamp(value, 50, 1000);
    }

    static int clampInteractionRestStableMs(int value)
    {
        return std::clamp(value, 100, 3000);
    }

    static int clampInteractionLiftDeltaThresholdMg(int value)
    {
        return std::clamp(value, 50, 1000);
    }

    static nlohmann::json defaultFanControlCurvePoints()
    {
        return nlohmann::json::array({
            nlohmann::json::array({ 35.0, 25 }),
            nlohmann::json::array({ 50.0, 40 }),
            nlohmann::json::array({ 65.0, 70 }),
            nlohmann::json::array({ 80.0, 100 }),
        });
    }

    static nlohmann::json normalizeFanControlCurvePoints(const nlohmann::json& value)
    {
        struct CurvePoint {
            double temperatureC = 0.0;
            int dutyPercent = 0;
        };

        if (!value.is_array()) {
            return defaultFanControlCurvePoints();
        }

        std::vector<CurvePoint> points;
        points.reserve(value.size());
        for (const auto& point : value) {
            if (!point.is_array() || point.size() != 2 || !point[0].is_number() || !point[1].is_number()) {
                return defaultFanControlCurvePoints();
            }
            points.push_back({
                .temperatureC = std::clamp(point[0].get<double>(), 0.0, 120.0),
                .dutyPercent = std::clamp(point[1].get<int>(), 0, 100),
            });
        }

        if (points.size() < 2) {
            return defaultFanControlCurvePoints();
        }

        std::sort(points.begin(), points.end(), [](const CurvePoint& left, const CurvePoint& right) {
            return left.temperatureC < right.temperatureC;
        });

        nlohmann::json normalized = nlohmann::json::array();
        for (const auto& point : points) {
            normalized.push_back(nlohmann::json::array({ point.temperatureC, point.dutyPercent }));
        }
        return normalized;
    }

    static std::vector<std::string> loadFontPaths(std::filesystem::path fontPath = "fonts")
    {
        std::vector<std::string> fontPaths = {};
        std::error_code ec;
        if (fontPath.empty() || !std::filesystem::exists(fontPath, ec) || !std::filesystem::is_directory(fontPath, ec)) {
            return fontPaths;
        }
        for (const auto& entry : std::filesystem::directory_iterator(fontPath, ec)) {
            if (ec) {
                return fontPaths;
            }
            if (entry.is_regular_file() && entry.path().extension() == ".ttf") {
                fontPaths.push_back(entry.path().string());
            } else if (entry.is_directory()) {
                auto subPaths = loadFontPaths(entry.path());
                fontPaths.insert(fontPaths.end(), subPaths.begin(), subPaths.end());
            }
        }
        return fontPaths;
    }
};

#endif
