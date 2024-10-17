#include <globalSettings.h>

std::vector<std::pair<GlobalSettings::SettingType, std::function<void()>>> GlobalSettings::settingChangeCallbacks = {};
std::mutex GlobalSettings::settingChangeMutex;
nlohmann::json GlobalSettings::settings = nlohmann::json();
std::unordered_map <GlobalSettings::SettingType, std::string> GlobalSettings::settingTypeStringMap = {
        {LOG_VERBOSITY, "logVerbosity"},
        {SELECTED_FONT_PATH, "selectedFontPath"},
        {LOG_LEVEL_PRINT, "logLevelP"},
        {LOG_LEVEL_FILE, "logLevelF"},
        {DEVELOPER_MODE_ENABLED, "developerModeEnabled"}
    };
std::unordered_map <std::string, GlobalSettings::SettingType> GlobalSettings::stringSettingTypeMap = {
        {"logVerbosity", LOG_VERBOSITY},
        {"selectedFontPath", SELECTED_FONT_PATH},
        {"logLevelP", LOG_LEVEL_PRINT},
        {"logLevelF", LOG_LEVEL_FILE},
        {"developerModeEnabled", DEVELOPER_MODE_ENABLED}
    };