/*
██╗      ██████╗  █████╗ ██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗ 
██║     ██╔═══██╗██╔══██╗██╔══██╗██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██║     ██║   ██║███████║██║  ██║█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██║     ██║   ██║██╔══██║██║  ██║██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
███████╗╚██████╔╝██║  ██║██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║     
╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

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
