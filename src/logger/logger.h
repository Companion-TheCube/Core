#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <source_location>

enum LogVerbosity {
    MINIMUM,
    TIMESTAMP,
    TIMESTAMP_AND_FILE,
    TIMESTAMP_AND_FILE_AND_LINE,
    TIMESTAMP_AND_FILE_AND_LINE_AND_FUNCTION,
    TIMESTAMP_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS
};

class ENTRY{
private:
    std::string message;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string messageFull;
public:
    virtual std::string getMessage() = 0;
    virtual std::string getTimestamp() = 0;
    virtual std::string getEntry() = 0;
    virtual std::string getMessageFull() = 0;
};

class CUBE_LOG_ENTRY: public ENTRY{
private:
    std::string message;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string messageFull;
public:
    static int logEntryCount;
    CUBE_LOG_ENTRY(std::string message, std::source_location* location, LogVerbosity verbosity);
    std::string getMessage();
    std::string getTimestamp();
    std::string getEntry();
    std::string getMessageFull();
};

class CUBE_ERROR: public ENTRY{
private:
    std::string message;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string messageFull;
public:
    static int errorCount;
    CUBE_ERROR(std::string message, std::source_location* location, LogVerbosity verbosity);
    std::string getMessage();
    std::string getEntry();
    std::string getTimestamp();
    std::string getMessageFull();
};

class CubeLog{
private:
    std::vector<CUBE_LOG_ENTRY> logEntries;
    std::vector<CUBE_ERROR> errors;
    std::mutex logMutex;
    LogVerbosity verbosity;
public:
    void log(std::string message, bool print, std::source_location location = std::source_location::current());
    void error(std::string message, std::source_location location = std::source_location::current());
    std::vector<CUBE_LOG_ENTRY> getLogEntries();
    std::vector<CUBE_ERROR> getErrors();
    std::vector<std::string> getLogEntriesAsStrings(bool fullMessages = true);
    std::vector<std::string> getErrorsAsStrings(bool fullMessages = true);
    std::vector<std::string> getLogsAndErrorsAsStrings(bool fullMessages = true);
    CubeLog(LogVerbosity verbosity = LogVerbosity::TIMESTAMP_AND_FILE_AND_LINE_AND_FUNCTION);
    void writeOutLogs();
    void setVerbosity(LogVerbosity verbosity);  
};

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp);
std::string getFileNameFromPath(std::string path);