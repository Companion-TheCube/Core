#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
#include <logger.h>
#include <filesystem>
#include "globalSettings.h"


class SettingsLoader{
private:
    nlohmann::json settings;
    std::string settingsFile;
    GlobalSettings *globalSettings;
public:
    SettingsLoader(GlobalSettings *globalSettings);
    ~SettingsLoader();
    bool loadSettings();
    bool saveSettings();
    bool setSetting(std::string key, std::string value);
    std::string getSetting(std::string key);
};