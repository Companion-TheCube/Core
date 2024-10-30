#include "loader.h"

SettingsLoader::SettingsLoader(GlobalSettings* settings)
{
    CubeLog::debug("Creating SettingsLoader");
    this->settings = nlohmann::json();
    this->settingsFile = "settings.json";
    this->globalSettings = settings;
    CubeLog::debug("SettingsLoader created");
}

SettingsLoader::~SettingsLoader()
{
    CubeLog::debug("Deleting SettingsLoader");
    this->saveSettings();
}

bool SettingsLoader::loadSettings()
{
    CubeLog::info("Attempting to load settings");
    // first check to see if the file exists
    if (!std::filesystem::exists(this->settingsFile)) {
        CubeLog::info("Settings file does not exist, creating new one");
        this->saveSettings();
    }
    CubeLog::debug("Settings file exists. Loading...");
    // now we open the file and load the settings
    std::ifstream file(this->settingsFile);
    if (!file.is_open()) {
        CubeLog::error("Failed to open settings file");
        return false;
    }
    CubeLog::debug("Settings file opened. Loading...");
    file >> this->settings;
    // iterate through all the settings and set them
    for (auto it = this->settings.begin(); it != this->settings.end(); ++it) {
        CubeLog::info("Setting " + it.key() + " to " + it.value().dump());
        this->globalSettings->setSetting(GlobalSettings::stringSettingTypeMap[it.key()], it.value());
    }
    CubeLog::debug("Settings loaded. Closing file...");
    file.close();
    CubeLog::info("Settings loaded");
    return true;
}

bool SettingsLoader::saveSettings()
{
    CubeLog::info("Attempting to save settings");
    // convert the GlobalSettings object to a json object
    this->settings = this->globalSettings->getSettings();
    std::ofstream file(this->settingsFile);
    if (!file.is_open()) {
        CubeLog::error("Failed to open settings file");
        return false;
    }
    file << this->settings.dump(4);
    file.close();
    CubeLog::info("Settings saved");
    return true;
}

bool SettingsLoader::setSetting(const std::string& key, const std::string& value)
{
    CubeLog::info("Setting " + key + " to " + value);
    this->settings[key] = value;
    return true;
}

std::string SettingsLoader::getSetting(const std::string& key)
{
    CubeLog::debug("Getting setting " + key);
    return this->settings[key];
}
