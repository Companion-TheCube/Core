#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
#include <logger.h>
#include <filesystem>
#include "globalSettings.h"


class SettingsLoader{
private:
    CubeLog *logger;
    nlohmann::json settings;
    std::string settingsFile;
    settings_ns::GlobalSettings *globalSettings;
public:
    SettingsLoader(CubeLog *logger, settings_ns::GlobalSettings *globalSettings);
    ~SettingsLoader();
    bool loadSettings();
    bool saveSettings();
    bool setSetting(std::string key, std::string value);
    std::string getSetting(std::string key);
};