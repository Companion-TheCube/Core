#pragma once
#include <logger.h>
#include <nlohmann/json.hpp>

struct GlobalSettings {
    static LogVerbosity logVerbosity; // = LogVerbosity::TIMESTAMP_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS;
    static std::vector<std::string> fontPaths;
    static std::string selectedFontPath;
    static LogLevel logLevelPrint;
    static LogLevel logLevelFile;

    bool setSetting(std::string key, nlohmann::json::value_type value){
        if(key == "logVerbosity"){
            // get the value from the void pointer
            int tempVal = value.get<int>();
            // convert to LogVerbosity
            this->logVerbosity = static_cast<LogVerbosity>(tempVal);
            return true;
        }
        if(key == "selectedFontPath"){
            // get the value from the void pointer
            std::string tempVal = value.get<std::string>();
            // convert to LogVerbosity
            this->selectedFontPath = tempVal;
            return true;
        }
        if(key == "logLevelP"){
            // get the value from the void pointer
            int tempVal = value.get<int>();
            // convert to LogVerbosity
            this->logLevelPrint = static_cast<LogLevel>(tempVal);
            return true;
        }
        if(key == "logLevelF"){
            // get the value from the void pointer
            int tempVal = value.get<int>();
            // convert to LogVerbosity
            this->logLevelFile = static_cast<LogLevel>(tempVal);
            return true;
        }
        return false;
    }
    nlohmann::json getSettings(){
        return nlohmann::json({
            {"logVerbosity", this->logVerbosity},
            {"selectedFontPath", this->selectedFontPath},
            {"logLevelP", this->logLevelPrint},
            {"logLevelF", this->logLevelFile}
        });
    }
    std::vector<std::string> getSettingsNames(){
        return {"logVerbosity", "selectedFontPath", "logLevelP", "logLevelF"};
    }
    enum FontPathIndices:unsigned int{
        ROBOTO_REGULAR,
        ROBOTO_BOLD,
        ROBOTO_ITALIC,
        ROBOTO_BOLDITALIC,
        ROBOTO_LIGHT,
        ROBOTO_LIGHTITALIC,
        ROBOTO_MEDIUM,
        ROBOTO_MEDIUMITALIC,
        ROBOTO_THIN,
        ROBOTO_THINITALIC,
        ROBOTO_BLACK,
        ROBOTO_BLACKITALIC,
        ASSISTANT_REGULAR,
        ASSISTANT_BOLD,
        ASSISTANT_EXTRABOLD,
        ASSISTANT_EXTRALIGHT,
        ASSISTANT_LIGHT,
        ASSISTANT_MEDIUM,
        ASSISTANT_SEMIBOLD,
        RADIOCANADABIG_BOLD,
        RADIOCANADABIG_BOLDITALIC,
        RADIOCANADABIG_ITALIC,
        RADIOCANADABIG_MEDIUM,
        RADIOCANADABIG_MEDIUMITALIC,
        RADIOCANADABIG_REGULAR,
        RADIOCANADABIG_SEMIBOLD,
        RADIOCANADABIG_SEMIBOLDITALIC,
        TOTAL_NUMBER_OF_INSTALLED_FONTS
    };
};