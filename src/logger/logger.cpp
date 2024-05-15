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

std::string CUBE_LOG_ENTRY::getEntry(){
    return this->getTimestamp() + ": " + this->getMessage();
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

std::string CUBE_ERROR::getEntry(){
    return this->getTimestamp() + ": " + this->getMessage();
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
    std::vector<std::string> logsAndErrorsAsStrings;
    std::vector<CUBE_LOG_ENTRY> logEntries = this->getLogEntries();
    std::vector<CUBE_ERROR> errors = this->getErrors();
    while(logEntries.size() > 0 || errors.size() > 0){
        if(logEntries.size() > 0 && errors.size() > 0){ // both have entries
            if(logEntries[0].getTimestamp() < errors[0].getTimestamp()){ // log entry is older
                logsAndErrorsAsStrings.push_back(logEntries[0].getEntry());
                logEntries.erase(logEntries.begin());
            }else{ // error is older
                logsAndErrorsAsStrings.push_back(errors[0].getEntry());
                errors.erase(errors.begin());
            }
        }else if(logEntries.size() > 0){ // only log entries left
            logsAndErrorsAsStrings.push_back(logEntries[0].getEntry());
            logEntries.erase(logEntries.begin());
        }else if(errors.size() > 0){ // only errors left
            logsAndErrorsAsStrings.push_back(errors[0].getEntry());
            errors.erase(errors.begin());
        }
    }
    return logsAndErrorsAsStrings;
}

void CubeLog::writeOutLogs(){
    // std::lock_guard<std::mutex> lock(this->logMutex);
    std::vector<std::string> logsAndErrors = this->getLogsAndErrorsAsStrings();   
    // write to file
    // We'll have to read through the file first to see if there are any existing logs.
    // Then we'll append the new logs to the end of the file.
    // We have to check if the file exists first.
    // We also have to check if the first log entry in logsAndErrors already exists in the file.
    
    // check if the file exists
    std::filesystem::path p("logs.txt");
    if(!std::filesystem::exists(p)){
        std::ofstream file("logs.txt");
        file.close();
    }
    std::ifstream file("logs.txt");
    std::string line;
    std::vector<std::string> existingLogs;
    if(file.is_open()){
        while(std::getline(file, line)){
            existingLogs.push_back(line);
        }
        file.close();
    }
    std::ofstream outFile("logs.txt"); // overwrites the file
    // get count of existing logs
    int existingLogsCount = existingLogs.size();
    // add to count of new logs
    int newLogsCount = logsAndErrors.size();
    // if the total number of logs is greater than 10000, remove the oldest logs
    if(existingLogsCount + newLogsCount > 10000){
        int numToRemove = existingLogsCount + newLogsCount - 10000;
        existingLogs.erase(existingLogs.begin(), existingLogs.begin() + numToRemove);
    }
    // write the existing logs to the file
    for(int i = 0; i < existingLogs.size(); i++){
        outFile<<existingLogs[i]<<std::endl;
    }
    // find the first log entry in logsAndErrors that is not in existingLogs
    int i = 0;
    while(i < logsAndErrors.size() && std::find(existingLogs.begin(), existingLogs.end(), logsAndErrors.at(i)) != existingLogs.end()){
        i++;
    }
    // write the new logs to the file
    for(; i < logsAndErrors.size(); i++){
        outFile<<logsAndErrors[i]<<std::endl;
    }
    outFile.close();
    
}

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp){
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream ss;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << milliseconds.count();
    
    return ss.str();
}