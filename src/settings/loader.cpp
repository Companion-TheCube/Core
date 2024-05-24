#include "loader.h"

SettingsLoader::SettingsLoader(CubeLog *logger, GlobalSettings *settings){
    this->logger = logger;
    this->settings = nlohmann::json();
    this->settingsFile = "settings.json";
    this->globalSettings = settings;
}

SettingsLoader::~SettingsLoader(){
    this->saveSettings();
}

bool SettingsLoader::loadSettings(){
    // first check to see if the file exists
    if(!std::filesystem::exists(this->settingsFile)){
        this->logger->log("Settings file does not exist, creating new one", true, std::source_location::current());
        this->saveSettings();
    }
    // now we open the file and load the settings
    std::ifstream file(this->settingsFile);
    if(!file.is_open()){
        this->logger->error("Failed to open settings file");
        return false;
    }
    file>>this->settings;
    // iterate through all the settings and set them
    for(auto it = this->settings.begin(); it != this->settings.end(); ++it){
        this->logger->log("Setting " + it.key() + " to " + it.value().dump(), true, std::source_location::current());
        this->globalSettings->setSetting(it.key(), it.value());
    }
    file.close();
    this->logger->log("Settings loaded", true, std::source_location::current());
    return true;
}

bool SettingsLoader::saveSettings(){
    this->logger->log("Attempting to save settings", true, std::source_location::current());
    // convert the GlobalSettings object to a json object
    this->settings = this->globalSettings->getSettings();
    std::ofstream file(this->settingsFile);
    if(!file.is_open()){
        this->logger->error("Failed to open settings file");
        return false;
    }
    file<<this->settings.dump(4);
    file.close();
    this->logger->log("Settings saved", true, std::source_location::current());
    return true;
}

bool SettingsLoader::setSetting(std::string key, std::string value){
    this->logger->log("Setting " + key + " to " + value, true, std::source_location::current());
    this->settings[key] = value;
    return true;
}

std::string SettingsLoader::getSetting(std::string key){
    return this->settings[key];
}

