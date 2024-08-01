#include "logger.h"
#ifdef _WIN32
#include "windows.h"
#include "psapi.h"
#endif


#define COUNTER_MOD 100
#define CUBE_LOG_ENTRY_MAX 50000 // maximum number of log entries in log file
#define LOG_WRITE_OUT_INTERVAL 300 // write out logs every 5 minutes
#define LOG_WRITE_OUT_COUNT 500 // write out logs every 500 logs
#define CUBE_LOG_MEMORY_LIMIT 1000 // maximum number of log entries in memory

std::string getMemoryFootprint();

unsigned int CUBE_LOG_ENTRY::logEntryCount = 0;

/**
 * @brief Construct a new cube log_entry entry object
 * 
 * @param message The message to log
 * @param location The source location of the log message
 * @param verbosity
 * @param level The log level of the message
 */
CUBE_LOG_ENTRY::CUBE_LOG_ENTRY(std::string message, std::source_location* location, LogVerbosity verbosity, LogLevel level){
    this->timestamp = std::chrono::system_clock::now();
    this->logEntryNumber = this->logEntryCount;
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

/**
 * @brief Destroy the cube log_entry entry object
 * 
 */
CUBE_LOG_ENTRY::~CUBE_LOG_ENTRY(){
}

/**
 * @brief Get the log message as a string
 * 
 * @return std::string 
 */
std::string CUBE_LOG_ENTRY::getMessage(){
    return this->message;
}

/**
 * @brief Get the log entry as a string with timestamp
 * 
 * @return std::string 
 */
std::string CUBE_LOG_ENTRY::getEntry(){
    return this->getTimestamp() + ": " + this->getMessage();
}

/**
 * @brief Get the timestamp as a string
 * 
 * @return std::string 
 */
std::string CUBE_LOG_ENTRY::getTimestamp(){
    return convertTimestampToString(this->timestamp);
}

/**
 * @brief Get the timestamp as a long
 * 
 * @return unsigned long long The timestamp as a long
 */
unsigned long long CUBE_LOG_ENTRY::getTimestampAsLong(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(this->timestamp.time_since_epoch()).count();
}

/**
 * @brief Get the full log message
 * 
 * @return std::string The full log message
 */
std::string CUBE_LOG_ENTRY::getMessageFull(){
    return this->messageFull;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////

LogVerbosity CubeLog::staticVerbosity = LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS;
LogLevel CubeLog::staticPrintLevel = LogLevel::LOGGER_INFO;
std::vector<CUBE_LOG_ENTRY> CubeLog::logEntries;
std::mutex CubeLog::logMutex;
bool CubeLog::consoleLoggingEnabled = true;
bool CubeLog::hasUnreadErrors_b = false;
bool CubeLog::hasUnreadLogs_b = false;
std::vector<unsigned int> CubeLog::readErrorIDs;
std::vector<unsigned int> CubeLog::readLogIDs;

/**
 * @brief Log a message
 * 
 * @param message The message to log
 * @param print If true, the message will be printed to the console
 * @param level The log level of the message
 * @param location The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::log(std::string message, bool print, LogLevel level, std::source_location location){
    CUBE_LOG_ENTRY entry = CUBE_LOG_ENTRY(message, &location, CubeLog::staticVerbosity, level);
    CubeLog::logEntries.push_back(entry);
    Color::Modifier colorDebug(Color::FG_GREEN);
    Color::Modifier colorInfo(Color::FG_WHITE);
    Color::Modifier colorWarning(Color::FG_MAGENTA);
    Color::Modifier colorError(Color::FG_LIGHT_YELLOW);
    Color::Modifier colorCritical(Color::FG_RED);
    Color::Modifier colorDefault(Color::FG_LIGHT_BLUE);
    if(print && level >= CubeLog::staticPrintLevel && CubeLog::consoleLoggingEnabled && message.length() < 1000){
        switch (level)
        {
        case LogLevel::LOGGER_DEBUG:
            std::cout << colorDebug << entry.getMessageFull() << std::endl;
            break;
        case LogLevel::LOGGER_INFO:
            std::cout << colorInfo << entry.getMessageFull() << std::endl;
            break;
        case LogLevel::LOGGER_WARNING:
            std::cout << colorWarning << entry.getMessageFull() << std::endl;
            break;
        case LogLevel::LOGGER_ERROR:
            std::cout << colorError << entry.getMessageFull() << std::endl;
            break;
        case LogLevel::LOGGER_CRITICAL:
            std::cout << colorCritical << entry.getMessageFull() << std::endl;
            break;
        default:
            std::cout << colorDefault << entry.getMessageFull() << std::endl;
            break;
        }
    }
}

/**
 * @brief Log a debug message
 * 
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::debug(std::string message, std::source_location location){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, LogLevel::LOGGER_DEBUG, location);
}

/**
 * @brief Log an error message
 * 
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::error(std::string message, std::source_location location){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, LogLevel::LOGGER_ERROR, location);
}

/**
 * @brief Log an informational message
 * 
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::info(std::string message, std::source_location location){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, LogLevel::LOGGER_INFO, location);
}

/**
 * @brief Log a warning message
 * 
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::warning(std::string message, std::source_location location){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, LogLevel::LOGGER_WARNING, location);
}

/**
 * @brief Log a critical message
 * 
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::critical(std::string message, std::source_location location){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, LogLevel::LOGGER_CRITICAL, location);
}

/**
 * @brief Construct a new CubeLog object
 * 
 * @param verbosity Determines the verbosity of the log messages.
 * @param printLevel Determines the level of log messages that will be printed to the console.
 * @param fileLevel Determines the level of log messages that will be written to the log file.
 */
CubeLog::CubeLog(LogVerbosity verbosity, LogLevel printLevel, LogLevel fileLevel){
    this->savingInProgress = false;
    CubeLog::staticVerbosity = verbosity;
    CubeLog::staticPrintLevel = printLevel;
    this->fileLevel = fileLevel;
    this->saveLogsThread = std::jthread([this]{
        this->saveLogsInterval();
    });
    CubeLog::log("Logger initialized", true);
}

/**
 * @brief Save logs to file every LOG_WRITE_OUT_INTERVAL seconds
 * 
 */
void CubeLog::saveLogsInterval(){
    
    while(true){
        for(size_t i = 0; i < LOG_WRITE_OUT_INTERVAL; i++){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if(this->logEntries.size() > LOG_WRITE_OUT_COUNT + this->savedLogsCount){
                this->writeOutLogs();
            }
            std::lock_guard<std::mutex> lock(this->saveLogsThreadRunMutex);
            if(!saveLogsThreadRun) break;
        }
        {
            std::lock_guard<std::mutex> lock(this->saveLogsThreadRunMutex);
            if(!saveLogsThreadRun) break;
        }
        this->writeOutLogs();
        this->purgeOldLogs();
        this->savedLogsCount = 0;
    }
}

/**
 * @brief Purge old logs from memory. The number of logs in memory will be limited to CUBE_LOG_MEMORY_LIMIT.
 * 
 */
void CubeLog::purgeOldLogs(){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    if(this->logEntries.size() > CUBE_LOG_MEMORY_LIMIT){
        this->logEntries.erase(this->logEntries.begin(), this->logEntries.begin() + this->logEntries.size() - CUBE_LOG_MEMORY_LIMIT);
    }
    this->logEntries.shrink_to_fit();
}

/**
 * @brief Destroy the CubeLog object
 * 
 */
CubeLog::~CubeLog(){
    CubeLog::log("Logger shutting down", true);
    std::lock_guard<std::mutex> lock(this->saveLogsThreadRunMutex);
    this->saveLogsThreadRun = false;
    Color::Modifier colorReset(Color::FG_DEFAULT);
    std::cout << colorReset;
    this->writeOutLogs();
}

/**
 * @brief Get the log entries
 * 
 * @param level The log level to get. If not provided, all log entries will be returned.
 * @return std::vector<CUBE_LOG_ENTRY> The log entries
 */
std::vector<CUBE_LOG_ENTRY> CubeLog::getLogEntries(LogLevel level){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    std::vector<CUBE_LOG_ENTRY> logEntries;
    for(int i = 0; i < this->logEntries.size(); i++){
        if(this->logEntries[i].level == level)
            logEntries.push_back(this->logEntries[i]);
    }
    return this->logEntries;
}

/**
 * @brief Get the log entries as strings
 * 
 * @param fullMessages If true, the full log messages will be returned. If false, only the message will be returned.
 * @return std::vector<std::string> The log entries as strings
 */
std::vector<std::string> CubeLog::getLogEntriesAsStrings(bool fullMessages){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    std::vector<std::string> logEntriesAsStrings;
    for(int i = 0; i < this->logEntries.size(); i++){
        if(fullMessages)
            logEntriesAsStrings.push_back(this->logEntries[i].getMessageFull());
        else
            logEntriesAsStrings.push_back(this->logEntries[i].getMessage());
    }
    return logEntriesAsStrings;
}

/**
 * @brief Get the logs and errors as strings
 * 
 * @param fullMessages If true, the full log messages will be returned. If false, only the message will be returned.
 * @return std::vector<std::string> The logs and errors as strings
 */
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

/**
 * @brief Write out logs to file
 * 
 */
void CubeLog::writeOutLogs(){
    std::cout << "Size of CubeLog: " << CubeLog::getSizeOfCubeLog() << std::endl;
    std::cout << "Memory footprint: " << getMemoryFootprint() << std::endl;
    std::lock_guard<std::mutex> lock(this->saveLogsMutex);
    if(this->savingInProgress) return;
    this->savingInProgress = true;
    std::cout << "Writing logs to file..." << std::endl;
    std::vector<std::string> logsAndErrors = this->getLogsAndErrorsAsStrings();   
    // write to file
    this->savedLogsCount = logsAndErrors.size();
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
    std::cout << "\nLogs written to file" << std::endl;
    this->savingInProgress = false;
}

/**
 * @brief Set the verbosity of the log messages
 * 
 * @param verbosity The verbosity of the log messages
 */
void CubeLog::setVerbosity(LogVerbosity verbosity){
    this->log("Setting verbosity to " + std::to_string(verbosity), true);
    CubeLog::staticVerbosity = verbosity;
}

/**
 * @brief Convert a timestamp to a string
 * 
 * @param timestamp The timestamp to convert
 * @return std::string The timestamp as a string
 */
std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp){
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream ss;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << milliseconds.count();
    
    return ss.str();
}

/**
 * @brief Get the file name from a path
 * 
 * @param path The path to get the file name from
 * @return std::string The file name
 */
std::string getFileNameFromPath(std::string path){
    std::filesystem::path p(path);
    return p.filename().string();
}

/**
 * @brief Set the log level for printing to the console and writing to the log file
 * 
 * @param printLevel The log level for printing to the console
 * @param fileLevel The log level for writing to the log file
 */
void CubeLog::setLogLevel(LogLevel printLevel, LogLevel fileLevel){
    CubeLog::staticPrintLevel = printLevel;
    this->fileLevel = fileLevel;
}

/**
 * @brief Set whether logging to the console is enabled
 * 
 * @param enabled If true, logging to the console is enabled
 */
void CubeLog::setConsoleLoggingEnabled(bool enabled){
    CubeLog::consoleLoggingEnabled = enabled;
}

/**
 * @brief Get the latest error
 * 
 * @return CUBE_LOG_ENTRY The latest error 
 */
CUBE_LOG_ENTRY CubeLog::getLatestError(){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    // find the latest error entry that is not in the readErrorIDs vector
    for(int i = CubeLog::logEntries.size() - 1; i >= 0; i--){
        if(CubeLog::logEntries[i].level == LogLevel::LOGGER_ERROR && std::find(CubeLog::readErrorIDs.begin(), CubeLog::readErrorIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readErrorIDs.end()){
            CubeLog::readErrorIDs.push_back(CubeLog::logEntries[i].logEntryNumber);
            return CubeLog::logEntries[i];
        }
    }
    return CUBE_LOG_ENTRY("No errors found", nullptr, LogVerbosity::MINIMUM, LogLevel::LOGGER_INFO);
}

/**
 * @brief Get the latest log
 * 
 * @return CUBE_LOG_ENTRY The latest log
 */
CUBE_LOG_ENTRY CubeLog::getLatestLog(){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    // find the latest log entry that is not an error and is not in the readLogIDs vector
    for(int i = CubeLog::logEntries.size() - 1; i >= 0; i--){
        if(CubeLog::logEntries[i].level != LogLevel::LOGGER_ERROR && std::find(CubeLog::readLogIDs.begin(), CubeLog::readLogIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readLogIDs.end()){
            CubeLog::readLogIDs.push_back(CubeLog::logEntries[i].logEntryNumber);
            return CubeLog::logEntries[i];
        }
    }
    return CUBE_LOG_ENTRY("No logs found", nullptr, LogVerbosity::MINIMUM, LogLevel::LOGGER_INFO);
}

/**
 * @brief Get the log latest entry
 */
CUBE_LOG_ENTRY CubeLog::getLatestEntry(){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    if(CubeLog::logEntries.size() > 0){
        return CubeLog::logEntries[CubeLog::logEntries.size() - 1];
        // add to readLogIDs or readErrorIDs depending on the level
        if (CubeLog::logEntries[CubeLog::logEntries.size() - 1].level == LogLevel::LOGGER_ERROR)
        {
            CubeLog::readErrorIDs.push_back(CubeLog::logEntries[CubeLog::logEntries.size() - 1].logEntryNumber);
        }
        else
        {
            CubeLog::readLogIDs.push_back(CubeLog::logEntries[CubeLog::logEntries.size() - 1].logEntryNumber);
        }
    }
    return CUBE_LOG_ENTRY("No logs found", nullptr, LogVerbosity::MINIMUM, LogLevel::LOGGER_INFO);
}

bool CubeLog::hasUnreadErrors(){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    bool hasUnreadErrors = false;
    // determine if any of the errors in the logEntries vector that are LogLevel::LOGGER_ERROR are not in the readErrorIDs vector
    for(int i = CubeLog::logEntries.size() - 1; i >= 0; i--){
        if(CubeLog::logEntries[i].level == LogLevel::LOGGER_ERROR && std::find(CubeLog::readErrorIDs.begin(), CubeLog::readErrorIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readErrorIDs.end()){
            hasUnreadErrors = true;
            break;
        }
    }
    return hasUnreadErrors;
}

bool CubeLog::hasUnreadLogs(){
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    bool hasUnreadLogs = false;
    // determine if any of the logs in the logEntries vector that are not LogLevel::LOGGER_ERROR are not in the readLogIDs vector
    for(int i = CubeLog::logEntries.size() - 1; i >= 0; i--){
        if(CubeLog::logEntries[i].level != LogLevel::LOGGER_ERROR && std::find(CubeLog::readLogIDs.begin(), CubeLog::readLogIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readLogIDs.end()){
            hasUnreadLogs = true;
            break;
        }
    }
    return hasUnreadLogs;
}

bool CubeLog::hasUnreadEntries(){
    return CubeLog::hasUnreadErrors() || CubeLog::hasUnreadLogs();
}





/**
 * @brief Get the name of the interface
 * 
 * @return std::string The name of the interface
 */
std::string CubeLog::getIntefaceName() const{
    return "Logger";
}

/**
 * @brief Get the endpoint data
 * 
 * @return HttpEndPointData_t The endpoint data
 */
HttpEndPointData_t CubeLog::getHttpEndpointData(){
    HttpEndPointData_t data;
    data.push_back({PUBLIC_ENDPOINT | POST_ENDPOINT, [](const httplib::Request &req, httplib::Response &res){
        // log info
        CubeLog::info("Logging message from endpoint", std::source_location::current());
        if((req.has_header("Content-Type") && req.get_header_value("Content-Type")=="application/json")){
            std::string message;
            std::string level;
            try{
                nlohmann::json received = nlohmann::json::parse(req.body);
                message = received["message"];
                level = received["level"];
            }catch(nlohmann::json::exception e){
                CubeLog::error(e.what(), std::source_location::current());
                // create json object with error message and return success = false
                nlohmann::json j;
                j["success"] = false;
                j["message"] = e.what();
                res.set_content(j.dump(), "application/json");
                return j.dump();
            }
            // convert level to int
            int level_int;
            try{
                level_int = std::stoi(level);
                if(level_int < 0 || level_int > 4) throw std::invalid_argument("Log level out of range. Must be between 0 and 4, inclusive.");
            }catch(std::invalid_argument e){
                CubeLog::error(e.what(), std::source_location::current());
                // create json object with error message and return success = false
                nlohmann::json j;
                j["success"] = false;
                j["message"] = e.what();
                res.set_content(j.dump(), "application/json");
                return j.dump();
            }
            // log message
            CubeLog::log(message, true, LogLevel(level_int), std::source_location::current());
            // create json object with success message and return success = true
            nlohmann::json j;
            j["success"] = true;
            j["message"] = "Logged message";
            res.set_content(j.dump(), "application/json");
            return j.dump();
        }else{
            // error
            CubeLog::error("Request is not a properly formatted json POST request.", std::source_location::current());
            // create json object with error message and return success = false
            nlohmann::json j;
            j["success"] = false;
            j["message"] = "Request is not a properly formatted json POST request.";
            res.set_content(j.dump(), "application/json");
            return j.dump();
        }
    }});
    return data;
}

/**
 * @brief Get the endpoint names
 * 
 * @return std::vector<std::string> The endpoint names
 */
std::vector<std::pair<std::string,std::vector<std::string>>> CubeLog::getHttpEndpointNamesAndParams(){
    std::vector<std::pair<std::string,std::vector<std::string>>> names;
    std::vector<std::string> logParams;
    logParams.push_back("message");
    logParams.push_back("level");
    names.push_back({"log", logParams});
    return names;
}

/**
 * @brief Get the size of the CubeLog in memory
 * 
 * @return std::string The size of the CubeLog
 */
std::string CubeLog::getSizeOfCubeLog(){
    size_t num_entries = CubeLog::logEntries.size();
    size_t total_size = 0;
    for(auto entry: CubeLog::logEntries){
        total_size += sizeof(entry) + entry.getMessage().size() + entry.getMessageFull().size();
    }
    return std::to_string(num_entries) + " entries, " + std::to_string(total_size) + " bytes";
}


std::string getMemoryFootprint(){
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return std::to_string(pmc.WorkingSetSize / 1024) + " KB";
#endif
#ifdef __linux__
    std::ifstream file("/proc/self/status");
    std::string line;
    while(std::getline(file, line)){
        if(line.find("VmRSS") != std::string::npos){
            std::string rss = line.substr(line.find(":") + 1);
            rss.erase(std::remove_if(rss.begin(), rss.end(), isspace), rss.end());
            return rss;
        }
    }
    return "0";
#endif
}