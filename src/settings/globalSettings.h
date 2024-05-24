#pragma once
#include <logger.h>

namespace settings_ns{
    struct GlobalSettings {
        LogVerbosity logVerbosity = LogVerbosity::TIMESTAMP_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS;
        bool setSetting(std::string key, nlohmann::json::value_type value){
            if(key == "logVerbosity"){
                // get the value from the void pointer
                int tempVal = value.get<int>();
                // convert to LogVerbosity
                this->logVerbosity = static_cast<LogVerbosity>(tempVal);
                return true;
            }
            return false;
        }
        nlohmann::json getSettings(){
            return nlohmann::json({
                {"logVerbosity", this->logVerbosity}
            });
        }
        std::vector<std::string> getSettingsNames(){
            return {"logVerbosity"};
        }
    };
}