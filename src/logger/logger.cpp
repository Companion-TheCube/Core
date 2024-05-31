// TODO: flush logs to file every so often (every 1000 logs or so) to prevent memory issues with large logs
// TODO: flush logs based on time (every 5 minutes or so)
// --- should probably keep the most recent 100 or so logs in memory so they can be available in the GUI

// TODO: replace logger with https://github.com/gabime/spdlog  ...maybe

#include "logger.h"

#define COUNTER_MOD 100
#define CUBE_LOG_ENTRY_MAX 50000

int CUBE_LOG_ENTRY::logEntryCount = 0;

CUBE_LOG_ENTRY::CUBE_LOG_ENTRY(std::string message, std::source_location* location, LogVerbosity verbosity, LogLevel level){
    this->timestamp = std::chrono::system_clock::now();
    this->logEntryCount++;
    this->message = message;
    this->level = level;
    std::string fileName = getFileNameFromPath(location->file_name());
    switch(verbosity){
        case LogVerbosity::MINIMUM:
            this->messageFull = message;
            break;
        case LogVerbosity::TIMESTAMP:
            this->messageFull = this->getTimestamp() + ": " + message;
            break;
        case LogVerbosity::TIMESTAMP_AND_LEVEL:
            this->messageFull = this->getTimestamp() + ": " + message + " (" + logLevelStrings[level] + ")";
            break;
        case LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE:
            this->messageFull = this->getTimestamp() + ": " + fileName + ": " + message + " (" + logLevelStrings[level] + ")";
            break;
        case LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE:
            this->messageFull = this->getTimestamp() + ": " + fileName + "(" + std::to_string(location->line()) + "): " + message + " (" + logLevelStrings[level] + ")";
            break;
        case LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION:
            this->messageFull = this->getTimestamp() + ": " + fileName + "(" + std::to_string(location->line()) + "): " + location->function_name() + ": " + message + " (" + logLevelStrings[level] + ")";
            break;
        default:
            this->messageFull = this->getTimestamp() + ": " + fileName + "(" + std::to_string(location->line()) + "): " + location->function_name() + ": " + message + " (" + std::to_string(CUBE_LOG_ENTRY::logEntryCount) + ")" + " (" + logLevelStrings[level] + ")";
            break;
    }
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

unsigned long long CUBE_LOG_ENTRY::getTimestampAsLong(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(this->timestamp.time_since_epoch()).count();
}

std::string CUBE_LOG_ENTRY::getMessageFull(){
    return this->messageFull;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////

void CubeLog::log(std::string message, bool print, LogLevel level, std::source_location location){
    CUBE_LOG_ENTRY entry = CUBE_LOG_ENTRY(message, &location, this->verbosity, level);
    Color::ExtendedModifier color(226);
    if(print && level >= this->printLevel){
        std::cout << color << entry.getMessageFull() << std::endl;
    }
    std::lock_guard<std::mutex> lock(this->logMutex);
    this->logEntries.push_back(entry);
}

void CubeLog::error(std::string message, std::source_location location){
    this->log(message, true, LogLevel::LOGGER_ERROR, location);
}

void CubeLog::info(std::string message, std::source_location location){
    this->log(message, true, LogLevel::LOGGER_INFO, location);
}

void CubeLog::warning(std::string message, std::source_location location){
    this->log(message, true, LogLevel::LOGGER_WARNING, location);
}

void CubeLog::critical(std::string message, std::source_location location){
    this->log(message, true, LogLevel::LOGGER_CRITICAL, location);
}

CubeLog::CubeLog(LogVerbosity verbosity, LogLevel printLevel, LogLevel fileLevel){
    this->verbosity = verbosity;
    this->printLevel = printLevel;
    this->fileLevel = fileLevel;
    CubeLog::log("Logger initialized", true);
}

std::vector<CUBE_LOG_ENTRY> CubeLog::getLogEntries(LogLevel level){
    std::lock_guard<std::mutex> lock(this->logMutex);
    std::vector<CUBE_LOG_ENTRY> logEntries;
    for(int i = 0; i < this->logEntries.size(); i++){
        if(this->logEntries[i].level == level)
            logEntries.push_back(this->logEntries[i]);
    }
    return this->logEntries;
}

std::vector<std::string> CubeLog::getLogEntriesAsStrings(bool fullMessages){
    std::lock_guard<std::mutex> lock(this->logMutex);
    std::vector<std::string> logEntriesAsStrings;
    for(int i = 0; i < this->logEntries.size(); i++){
        if(fullMessages)
            logEntriesAsStrings.push_back(this->logEntries[i].getMessageFull());
        else
            logEntriesAsStrings.push_back(this->logEntries[i].getMessage());
    }
    return logEntriesAsStrings;
}

std::vector<std::string> CubeLog::getLogsAndErrorsAsStrings(bool fullMessages){
    // time how long this function takes
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::string> logsAndErrorsAsStrings;
    std::vector<CUBE_LOG_ENTRY> logEntries = this->getLogEntries(this->fileLevel);
    std::vector<std::pair<unsigned long long, std::string>> logsAndErrorsAsPairs;
    for(auto entry : logEntries){
        logsAndErrorsAsPairs.push_back({entry.getTimestampAsLong(), entry.getMessageFull()});
    }
    std::stable_sort(logsAndErrorsAsPairs.begin(), logsAndErrorsAsPairs.end());
    for(auto pair : logsAndErrorsAsPairs){
        logsAndErrorsAsStrings.push_back(pair.second);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "getLogsAndErrorsAsStrings took " << duration.count() << " seconds" << std::endl;
    return logsAndErrorsAsStrings;
}

void CubeLog::writeOutLogs(){
    std::cout << "Writing logs to file..." << std::endl;
    std::vector<std::string> logsAndErrors = this->getLogsAndErrorsAsStrings();   
    // write to file
    std::filesystem::path p("logs.txt");
    if(!std::filesystem::exists(p)){
        std::ofstream file("logs.txt");
        file.close();
    }
    std::ifstream file("logs.txt");
    std::string line;
    std::vector<std::string> existingLogs;
    size_t counter = 0;
    if(file.is_open()){
        while(std::getline(file, line)){
            existingLogs.push_back(line);
            counter++;
            if(counter % COUNTER_MOD == 0) std::cout << ".";
        }
        file.close();
    }
    std::ofstream outFile("logs.txt"); // overwrites the file
    // get count of existing logs
    long long existingLogsCount = existingLogs.size();
    // add to count of new logs
    long long newLogsCount = logsAndErrors.size();
    // if the total number of logs is greater than CUBE_LOG_ENTRY_MAX, remove the oldest logs
    counter = 0;
    if(existingLogsCount + newLogsCount > CUBE_LOG_ENTRY_MAX){
        long long numToRemove = existingLogsCount + newLogsCount - CUBE_LOG_ENTRY_MAX;
        if(numToRemove > existingLogsCount){
            existingLogs.clear();
            numToRemove = 0;
        }else{
            existingLogs.erase(existingLogs.begin(), existingLogs.begin() + numToRemove);
        }
        counter++;
        if(counter % COUNTER_MOD == 0) std::cout << ".";
    }
    // write the existing logs to the file
    counter = 0;
    for(size_t i = 0; i < existingLogs.size(); i++){
        outFile<<existingLogs[i]<<std::endl;
        counter++;
        if(counter % COUNTER_MOD == 0) std::cout << ".";
    }
    // find the first log entry in logsAndErrors that is not in existingLogs
    size_t i = 0;
    counter = 0;
    while(i < logsAndErrors.size() && std::find(existingLogs.begin(), existingLogs.end(), logsAndErrors.at(i)) != existingLogs.end()){
        i++;
        counter++;
        if(counter % COUNTER_MOD == 0) std::cout << ".";
    }
    // write the new logs to the file
    counter = 0;
    for(; i < logsAndErrors.size(); i++){
        outFile<<logsAndErrors[i]<<std::endl;
        counter++;
        if(counter % COUNTER_MOD == 0) std::cout << ".";
    }
    outFile.close();
    
}

void CubeLog::setVerbosity(LogVerbosity verbosity){
    this->log("Setting verbosity to " + std::to_string(verbosity), true);
    this->verbosity = verbosity;
}

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp){
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream ss;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << milliseconds.count();
    
    return ss.str();
}

std::string getFileNameFromPath(std::string path){
    std::filesystem::path p(path);
    return p.filename().string();
}

void CubeLog::setLogLevel(LogLevel printLevel, LogLevel fileLevel){
    this->printLevel = printLevel;
    this->fileLevel = fileLevel;
}