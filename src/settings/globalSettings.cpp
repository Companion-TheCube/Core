#include <globalSettings.h>

Logger::LogVerbosity GlobalSettings::logVerbosity = Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS;
std::string GlobalSettings::selectedFontPath = "";
Logger::LogLevel GlobalSettings::logLevelPrint = Logger::LogLevel::LOGGER_INFO;
Logger::LogLevel GlobalSettings::logLevelFile = Logger::LogLevel::LOGGER_INFO;
std::vector<std::string> GlobalSettings::fontPaths = {};
std::vector<std::pair<std::string, std::function<void()>>> GlobalSettings::settingChangeCallbacks = {};
std::mutex GlobalSettings::settingChangeMutex;