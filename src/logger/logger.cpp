#include "logger.h"

int CUBE_LOG_ENTRY::logEntryCount = 0;
int CUBE_ERROR::errorCount = 0;

CUBE_LOG_ENTRY::CUBE_LOG_ENTRY(std::string message){
    this->message = message;
    this->logEntryCount++;
    this->timestamp = std::chrono::system_clock::now();
}

std::string CUBE_LOG_ENTRY::getMessage(){
    return this->message;
}

std::string CUBE_LOG_ENTRY::getTimestamp(){
    return convertTimestampToString(this->timestamp);
}

CUBE_ERROR::CUBE_ERROR(std::string message){
    this->message = message;
    this->errorCount++;
    this->timestamp = std::chrono::system_clock::now();
}

std::string CUBE_ERROR::getMessage(){
    return this->message;
}

std::string CUBE_ERROR::getTimestamp(){
    return convertTimestampToString(this->timestamp);
}

void CubeLog::log(std::string message, bool print){
    CUBE_LOG_ENTRY entry = CUBE_LOG_ENTRY(message);
    if(print){
        std::cout<<entry.getTimestamp()<<": "<<entry.getMessage()<<std::endl;
    }
    std::lock_guard<std::mutex> lock(this->logMutex);
    this->logEntries.push_back(entry);
}

void CubeLog::error(std::string message){
    CUBE_ERROR error = CUBE_ERROR(message);
    std::cerr<<error.getTimestamp()<<": "<<error.getMessage()<<std::endl;
    std::lock_guard<std::mutex> lock(this->logMutex);
    this->errors.push_back(error);
}

CubeLog::CubeLog(){
    CubeLog::log("Logger initialized", true);
}

std::vector<CUBE_LOG_ENTRY> CubeLog::getLogEntries(){
    std::lock_guard<std::mutex> lock(this->logMutex);
    return this->logEntries;
}

std::vector<CUBE_ERROR> CubeLog::getErrors(){
    std::lock_guard<std::mutex> lock(this->logMutex);
    return this->errors;
}

std::vector<std::string> CubeLog::getLogEntriesAsStrings(){
    std::lock_guard<std::mutex> lock(this->logMutex);
    std::vector<std::string> logEntriesAsStrings;
    for(int i = 0; i < this->logEntries.size(); i++){
        logEntriesAsStrings.push_back(this->logEntries[i].getMessage());
    }
    return logEntriesAsStrings;
}

std::vector<std::string> CubeLog::getErrorsAsStrings(){
    std::lock_guard<std::mutex> lock(this->logMutex);
    std::vector<std::string> errorsAsStrings;
    for(int i = 0; i < this->errors.size(); i++){
        errorsAsStrings.push_back(this->errors[i].getMessage());
    }
    return errorsAsStrings;
}

std::vector<std::string> CubeLog::getLogsAndErrorsAsStrings(){
    std::lock_guard<std::mutex> lock(this->logMutex);
    std::vector<std::string> logsAndErrorsAsStrings;
    // sort log entries and errors by timestamp
    std::vector<CUBE_LOG_ENTRY> logEntries = this->getLogEntries();
    std::vector<CUBE_ERROR> errors = this->getErrors();
    std::vector<CUBE_LOG_ENTRY> logEntriesSorted;
    std::vector<CUBE_ERROR> errorsSorted;
    while(logEntries.size() > 0 || errors.size() > 0){
        if(logEntries.size() > 0 && errors.size() > 0){
            if(logEntries[0].getTimestamp() < errors[0].getTimestamp()){
                logEntriesSorted.push_back(logEntries[0]);
                logEntries.erase(logEntries.begin());
            }else{
                errorsSorted.push_back(errors[0]);
                errors.erase(errors.begin());
            }
        }else if(logEntries.size() > 0){
            logEntriesSorted.push_back(logEntries[0]);
            logEntries.erase(logEntries.begin());
        }else if(errors.size() > 0){
            errorsSorted.push_back(errors[0]);
            errors.erase(errors.begin());
        }
    }
    for(int i = 0; i < logEntriesSorted.size(); i++){
        logsAndErrorsAsStrings.push_back(logEntriesSorted[i].getMessage());
    }
    for(int i = 0; i < errorsSorted.size(); i++){
        logsAndErrorsAsStrings.push_back(errorsSorted[i].getMessage());
    }
    return logsAndErrorsAsStrings;
}

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp){
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}