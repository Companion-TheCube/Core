#include <globalSettings.h>

LogVerbosity GlobalSettings::logVerbosity = LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS;
std::string GlobalSettings::selectedFontPath = "";
LogLevel GlobalSettings::logLevelPrint = LogLevel::LOGGER_INFO;
LogLevel GlobalSettings::logLevelFile = LogLevel::LOGGER_INFO;
std::vector<std::string> GlobalSettings::fontPaths = {};
std::vector<std::pair<std::string, std::function<void()>>> GlobalSettings::settingChangeCallbacks = {};