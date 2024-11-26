#pragma once
#ifndef GLOBAL_SETTINGS_H
#define GLOBAL_SETTINGS_H
#ifndef UTILS_H
#include "utils.h"
#endif
#include <filesystem>
#include <logger.h>
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
        NOTIFICATION_SOUND,
        ALARM_SOUND,
        VOICE_COMMAND_SOUND,
        NFC_ENABLED,
        MICROPHONE_ENABLED,
        PRESENCE_DETECTION_ENABLED,
        EMOTION_CURIOSITY,
        EMOTION_PLAYFULNESS,
        EMOTION_EMPATHY,
        EMOTION_ASSERTIVENESS,
        EMOTION_ATTENTIVENESS,
        EMOTION_CAUTION,
        EMOTION_ANNOYANCE,

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
        bool defaultFound = false;
        for (auto fontPath : loadFontPaths()) {
            // set the default font to Roboto-Regular.ttf
            if (fontPath.find("Roboto-Regular.ttf") != std::string::npos) {
                lock.unlock();
                GlobalSettings::setSetting(SettingType::SELECTED_FONT_PATH, fontPath);
                defaultFound = true;
            }
        }
        if (!defaultFound) {
            CubeLog::fatal("Default font not found.");
        }
        // set the default log verbosity to TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE
        GlobalSettings::setSetting(SettingType::LOG_VERBOSITY, Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE);
        // set the default log level for printing to INFO
        GlobalSettings::setSetting(SettingType::LOG_LEVEL_PRINT, Logger::LogLevel::LOGGER_INFO);
        // set the default log level for writing to file to INFO
        GlobalSettings::setSetting(SettingType::LOG_LEVEL_FILE, Logger::LogLevel::LOGGER_INFO);
        // set the default developer mode to false
        GlobalSettings::setSetting(SettingType::DEVELOPER_MODE_ENABLED, false);
        // set the default CPU and memory display to false
        GlobalSettings::setSetting(SettingType::CPU_AND_MEMORY_DISPLAY_ENABLED, false);
        // set the default SSH to false
        GlobalSettings::setSetting(SettingType::SSH_ENABLED, false);
        // set the default WiFi to true
        GlobalSettings::setSetting(SettingType::WIFI_ENABLED, true);
        // set the default BT to true
        GlobalSettings::setSetting(SettingType::BT_ENABLED, true);
        // set the default idle animation to true
        GlobalSettings::setSetting(SettingType::IDLE_ANIMATION_ENABLED, true);
        // set the default screen brightness to 100
        GlobalSettings::setSetting(SettingType::SCREEN_BRIGHTNESS, 100);
        // set the default screen auto off to true
        GlobalSettings::setSetting(SettingType::SCREEN_AUTO_OFF, true);
        // set the default screen auto off time to 5 minutes
        GlobalSettings::setSetting(SettingType::SCREEN_AUTO_OFF_TIME, 5);
        // set the default notifications from network to true
        GlobalSettings::setSetting(SettingType::NOTIFICATIONS_FROM_NETWORK_ENABLED, true);
        // set the default system volume to 100
        GlobalSettings::setSetting(SettingType::SYSTEM_VOLUME, 100);
        // set the default notification sound to default
        GlobalSettings::setSetting(SettingType::NOTIFICATION_SOUND, "default");
        // set the default alarm sound to default
        GlobalSettings::setSetting(SettingType::ALARM_SOUND, "default");
        // set the default voice command sound to default
        GlobalSettings::setSetting(SettingType::VOICE_COMMAND_SOUND, "default");
        // set the default NFC to true
        GlobalSettings::setSetting(SettingType::NFC_ENABLED, true);
        // set the default microphone to true
        GlobalSettings::setSetting(SettingType::MICROPHONE_ENABLED, true);
        // set the default presence detection to true
        GlobalSettings::setSetting(SettingType::PRESENCE_DETECTION_ENABLED, true);
        // set the default emotion curiosity to 80
        GlobalSettings::setSetting(SettingType::EMOTION_CURIOSITY, 80);
        // set the default emotion playfulness to 70
        GlobalSettings::setSetting(SettingType::EMOTION_PLAYFULNESS, 70);
        // set the default emotion empathy to 85
        GlobalSettings::setSetting(SettingType::EMOTION_EMPATHY, 85);
        // set the default emotion assertiveness to 60
        GlobalSettings::setSetting(SettingType::EMOTION_ASSERTIVENESS, 60);
        // set the default emotion attentiveness to 90
        GlobalSettings::setSetting(SettingType::EMOTION_ATTENTIVENESS, 90);
        // set the default emotion caution to 50
        GlobalSettings::setSetting(SettingType::EMOTION_CAUTION, 50);
        // set the default emotion annoyance to 0
        GlobalSettings::setSetting(SettingType::EMOTION_ANNOYANCE, 0);
    }

    static void setSettingCB(SettingType key, std::function<void()> callback)
    {
        settingChangeCallbacks.push_back({ key, callback });
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
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        settings[settingTypeStringMap[key]] = value;
        callSettingCB(key);
        return true;
    }
    static nlohmann::json getSettings()
    {
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        return settings;
    }
    static nlohmann::json::value_type getSetting(SettingType key)
    {
        std::unique_lock<std::mutex> lock(settingChangeMutex);
        return settings[settingTypeStringMap[key]];
    }
    template <typename T>
    static T getSettingOfType(SettingType key)
    {
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

    static std::vector<std::string> loadFontPaths(std::filesystem::path fontPath = "./fonts")
    {
        std::vector<std::string> fontPaths = {};
        for (const auto& entry : std::filesystem::directory_iterator(fontPath)) {
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