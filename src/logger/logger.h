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
    TIMESTAMP_AND_LEVEL,
    TIMESTAMP_AND_LEVEL_AND_FILE,
    TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE,
    TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION,
    TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS,
    LOGVERBOSITYCOUNT
};

enum LogLevel {
    LOGGER_INFO,
    LOGGER_WARNING,
    LOGGER_ERROR,
    LOGGER_CRITICAL,
    LOGGER_LOGLEVELCOUNT
};

const std::string logLevelStrings[] = {
    "INFO",
    "WARNING",
    "ERROR",
    "CRITICAL"
};

class CUBE_LOG_ENTRY{
private:
    std::string message;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string messageFull;
public:
    LogLevel level;
    static int logEntryCount;
    CUBE_LOG_ENTRY(std::string message, std::source_location* location, LogVerbosity verbosity, LogLevel level = LogLevel::LOGGER_INFO);
    std::string getMessage();
    std::string getTimestamp();
    std::string getEntry();
    std::string getMessageFull();
    unsigned long long getTimestampAsLong();
};

class CubeLog{
private:
    std::vector<CUBE_LOG_ENTRY> logEntries;
    std::mutex logMutex;
    LogVerbosity verbosity;
    LogLevel printLevel;
    LogLevel fileLevel;
public:
    void log(std::string message, bool print, LogLevel level = LogLevel::LOGGER_INFO, std::source_location location = std::source_location::current());
    void error(std::string message, std::source_location location = std::source_location::current());
    void info(std::string message, std::source_location location = std::source_location::current());
    void warning(std::string message, std::source_location location = std::source_location::current());
    void critical(std::string message, std::source_location location = std::source_location::current());
    std::vector<CUBE_LOG_ENTRY> getLogEntries(LogLevel level = LogLevel::LOGGER_INFO);
    std::vector<std::string> getLogEntriesAsStrings(bool fullMessages = true);
    std::vector<std::string> getErrorsAsStrings(bool fullMessages = true);
    std::vector<std::string> getLogsAndErrorsAsStrings(bool fullMessages = true);
    CubeLog(LogVerbosity verbosity = LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION, LogLevel printLevel = LogLevel::LOGGER_INFO, LogLevel fileLevel = LogLevel::LOGGER_INFO);
    void writeOutLogs();
    void setVerbosity(LogVerbosity verbosity);
    void setLogLevel(LogLevel printLevel, LogLevel fileLevel);
};

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp);
std::string getFileNameFromPath(std::string path);