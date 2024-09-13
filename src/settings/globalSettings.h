#pragma once
#include <logger.h>
#include <nlohmann/json.hpp>
#include <filesystem>

struct GlobalSettings {
    static LogVerbosity logVerbosity;
    static std::vector<std::string> fontPaths;
    static std::string selectedFontPath;
    static LogLevel logLevelPrint;
    static LogLevel logLevelFile;
    static std::vector<std::pair<std::string, std::function<void()>>> settingChangeCallbacks;

    GlobalSettings(){
        GlobalSettings::fontPaths = loadFontPaths();
        for(auto fontPath : GlobalSettings::fontPaths){
            // set the default font to Roboto-Regular.ttf
            if(fontPath.find("Roboto-Regular.ttf") != std::string::npos){
                GlobalSettings::setSetting("selectedFontPath", fontPath);
            }
        }
    }

    static void setSettingCB(std::string key, std::function<void()> callback){
        // find the callback and remove it
        for(auto it = settingChangeCallbacks.begin(); it != settingChangeCallbacks.end(); it++){
            if(it->first == key){
                settingChangeCallbacks.erase(it);
                break;
            }
        }
        settingChangeCallbacks.push_back({key, callback});
    }

    static void callSettingCB(std::string key){
        CubeLog::debug("Calling setting change callback for " + key);
        for(auto cb : settingChangeCallbacks){
            if(cb.first == key){
                cb.second();
            }
        }
    }

    static bool setSetting(std::string key, nlohmann::json::value_type value){
        if(key == "logVerbosity"){
            int tempVal = value.get<int>();
            GlobalSettings::logVerbosity = static_cast<LogVerbosity>(tempVal);
            GlobalSettings::callSettingCB(key);
            return true;
        }
        if(key == "selectedFontPath"){
            std::string tempVal = value.get<std::string>();
            GlobalSettings::selectedFontPath = tempVal;
            GlobalSettings::callSettingCB(key);
            return true;
        }
        if(key == "logLevelP"){
            int tempVal = value.get<int>();
            GlobalSettings::logLevelPrint = static_cast<LogLevel>(tempVal);
            GlobalSettings::callSettingCB(key);
            return true;
        }
        if(key == "logLevelF"){
            int tempVal = value.get<int>();
            GlobalSettings::logLevelFile = static_cast<LogLevel>(tempVal);
            GlobalSettings::callSettingCB(key);
            return true;
        }
        return false;
    }
    static nlohmann::json getSettings(){
        return nlohmann::json({
            {"logVerbosity", GlobalSettings::logVerbosity},
            {"selectedFontPath", GlobalSettings::selectedFontPath},
            {"logLevelP", GlobalSettings::logLevelPrint},
            {"logLevelF", GlobalSettings::logLevelFile}
        });
    }
    static std::vector<std::string> getSettingsNames(){
        return {"logVerbosity", "selectedFontPath", "logLevelP", "logLevelF"};
    }

    static std::vector<std::string> loadFontPaths(std::filesystem::path fontPath = "./fonts"){
        std::vector<std::string> fontPaths = {};
        for (const auto & entry : std::filesystem::directory_iterator(fontPath)){
            if(entry.is_regular_file() && entry.path().extension() == ".ttf"){
                fontPaths.push_back(entry.path().string()); 
            }else if(entry.is_directory()){
                auto subPaths = loadFontPaths(entry.path());
                fontPaths.insert(fontPaths.end(), subPaths.begin(), subPaths.end());
            }
        }
        return fontPaths;
    }
    static std::string toString(){
        std::string output = "";
        switch(GlobalSettings::logVerbosity){
            case LogVerbosity::MINIMUM:
                output += "Log Verbosity: Message only\n";
                break;
            case LogVerbosity::TIMESTAMP:
                output += "Log Verbosity: Timestamp and message\n";
                break;
            case LogVerbosity::TIMESTAMP_AND_LEVEL:
                output += "Log Verbosity: Timestamp, log level, and message\n";
                break;
            case LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE:
                output += "Log Verbosity: Timestamp, log level, source file, and message\n";
                break;
            case LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE:
                output += "Log Verbosity: Timestamp, log level, source file, line number, and message\n";
                break;
            case LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION:
                output += "Log Verbosity: Timestamp, log level, source file, line number, function name, and message\n";
                break;
            case LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS:
                output += "Log Verbosity: Timestamp, log level, source file, line number, function name, number of logs, and message\n";
                break;
            default:
                output += "Log Verbosity: UNKNOWN\n";
        }
        switch(GlobalSettings::logLevelPrint){
            case LogLevel::LOGGER_INFO:
                output += "Log Level for printing: INFO\n";
                break;
            case LogLevel::LOGGER_WARNING:
                output += "Log Level for printing: WARNING\n";
                break;
            case LogLevel::LOGGER_ERROR:
                output += "Log Level for printing: ERROR\n";
                break;
            case LogLevel::LOGGER_CRITICAL:
                output += "Log Level for printing: CRITICAL\n";
                break;
            case LogLevel::LOGGER_OFF:
                output += "Log Level for printing: OFF\n";
                break;
            case LogLevel::LOGGER_DEBUG:
                output += "Log Level for printing: DEBUG\n";
                break;
            default:
                output += "Log Level for printing: UNKNOWN\n";
        }
        switch(GlobalSettings::logLevelFile){
            case LogLevel::LOGGER_INFO:
                output += "Log Level for log file: INFO\n";
                break;
            case LogLevel::LOGGER_WARNING:
                output += "Log Level for log file: WARNING\n";
                break;
            case LogLevel::LOGGER_ERROR:
                output += "Log Level for log file: ERROR\n";
                break;
            case LogLevel::LOGGER_CRITICAL:
                output += "Log Level for log file: CRITICAL\n";
                break;
            case LogLevel::LOGGER_OFF:
                output += "Log Level for log file: OFF\n";
                break;
            case LogLevel::LOGGER_DEBUG:
                output += "Log Level for log file: DEBUG\n";
                break;
            default:
                output += "Log Level for log file: UNKNOWN\n";
        }
        auto fontpath = std::filesystem::path(GlobalSettings::selectedFontPath);
        output += "Selected Font: " + std::string(fontpath.filename().string()) + "\n";
        return output;
    }
};