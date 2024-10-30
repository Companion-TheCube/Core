#pragma once
#include "globalSettings.h"
#include <filesystem>
#include <fstream>
#include <logger.h>
#include <nlohmann/json.hpp>

class SettingsLoader {
private:
    nlohmann::json settings;
    std::string settingsFile;
    GlobalSettings* globalSettings;

public:
    SettingsLoader(GlobalSettings* globalSettings);
    ~SettingsLoader();
    bool loadSettings();
    bool saveSettings();
    bool setSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key);
};