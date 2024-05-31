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
        this->logger->info("Settings file does not exist, creating new one");
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
        this->logger->info("Setting " + it.key() + " to " + it.value().dump());
        this->globalSettings->setSetting(it.key(), it.value());
    }
    file.close();
    this->logger->info("Settings loaded");
    return true;
}

bool SettingsLoader::saveSettings(){
    this->logger->info("Attempting to save settings");
    // convert the GlobalSettings object to a json object
    this->settings = this->globalSettings->getSettings();
    std::ofstream file(this->settingsFile);
    if(!file.is_open()){
        this->logger->error("Failed to open settings file");
        return false;
    }
    file<<this->settings.dump(4);
    file.close();
    this->logger->info("Settings saved");
    return true;
}

bool SettingsLoader::setSetting(std::string key, std::string value){
    this->logger->info("Setting " + key + " to " + value);
    this->settings[key] = value;
    return true;
}

std::string SettingsLoader::getSetting(std::string key){
    return this->settings[key];
}

