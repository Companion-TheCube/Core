#pragma once
#ifndef LOGGER_H
#define LOGGER_H
#define WIN32_LEAN_AND_MEAN
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
#include "../api/api_i.h"

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
    LOGGER_DEBUG,
    LOGGER_INFO,
    LOGGER_WARNING,
    LOGGER_ERROR,
    LOGGER_CRITICAL,
    LOGGER_LOGLEVELCOUNT
};

const std::string logLevelStrings[] = {
    "DEBUG",
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
    unsigned int logEntryNumber;
    LogLevel level;
    static unsigned int logEntryCount;
    CUBE_LOG_ENTRY(std::string message, std::source_location* location, LogVerbosity verbosity, LogLevel level = LogLevel::LOGGER_INFO);
    std::string getMessage();
    std::string getTimestamp();
    std::string getEntry();
    std::string getMessageFull();
    unsigned long long getTimestampAsLong();
};

class CubeLog : public I_API_Interface{
private:
    static std::vector<CUBE_LOG_ENTRY> logEntries;
    static std::mutex logMutex;
    static LogVerbosity staticVerbosity;
    static LogLevel staticPrintLevel;
    static bool consoleLoggingEnabled;
    LogLevel fileLevel;
    bool savingInProgress;
    void saveLogsInterval();
    std::jthread saveLogsThread;
    std::mutex saveLogsMutex;
    unsigned long long savedLogsCount = 0;
    bool saveLogsThreadRun = true;
    std::mutex saveLogsThreadRunMutex;
    void purgeOldLogs();
    static bool hasUnreadErrors_b, hasUnreadLogs_b;
    static std::vector<unsigned int> readErrorIDs, readLogIDs;
public:
    static void log(std::string message, bool print, LogLevel level = LogLevel::LOGGER_INFO, std::source_location location = std::source_location::current());
    static void debug(std::string message, std::source_location location = std::source_location::current());
    static void error(std::string message, std::source_location location = std::source_location::current());
    static void info(std::string message, std::source_location location = std::source_location::current());
    static void warning(std::string message, std::source_location location = std::source_location::current());
    static void critical(std::string message, std::source_location location = std::source_location::current());
    std::vector<CUBE_LOG_ENTRY> getLogEntries(LogLevel level = LogLevel::LOGGER_INFO);
    std::vector<std::string> getLogEntriesAsStrings(bool fullMessages = true);
    std::vector<std::string> getErrorsAsStrings(bool fullMessages = true);
    std::vector<std::string> getLogsAndErrorsAsStrings(bool fullMessages = true);
    CubeLog(LogVerbosity verbosity = LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION, LogLevel printLevel = LogLevel::LOGGER_INFO, LogLevel fileLevel = LogLevel::LOGGER_INFO);
    ~CubeLog();
    void writeOutLogs();
    void setVerbosity(LogVerbosity verbosity);
    void setLogLevel(LogLevel printLevel, LogLevel fileLevel);
    static void setConsoleLoggingEnabled(bool enabled);
    static CUBE_LOG_ENTRY getLatestError();
    static CUBE_LOG_ENTRY getLatestLog();
    static CUBE_LOG_ENTRY getLatestEntry();
    static bool hasUnreadErrors();
    static bool hasUnreadLogs();
    static bool hasUnreadEntries();

    // API Interface
    std::string getIntefaceName() const;
    EndPointData_t getEndpointData();
    std::vector<std::pair<std::string,std::vector<std::string>>> getEndpointNamesAndParams();
};

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp);
std::string getFileNameFromPath(std::string path);

namespace Color {
    enum Code {
        FG_RED          = 31,
        FG_GREEN        = 32,
        FG_BLUE         = 34,
        FG_MAGENTA      = 35,
        FG_CYAN         = 36,
        FG_LIGHT_GRAY   = 37,
        FG_YELLOW       = 33,
        FG_DARK_GRAY    = 90,
        FG_LIGHT_RED    = 91,
        FG_LIGHT_GREEN  = 92,
        FG_LIGHT_YELLOW = 93,
        FG_LIGHT_BLUE   = 94,
        FG_LIGHT_MAGENTA= 95,
        FG_LIGHT_CYAN   = 96,
        FG_WHITE        = 97,
        FG_DEFAULT      = 39,
        BG_RED          = 41,
        BG_GREEN        = 42,
        BG_BLUE         = 44,
        BG_MAGENTA      = 45,
        BG_CYAN         = 46,
        BG_LIGHT_GRAY   = 47,
        BG_DARK_GRAY    = 100,
        BG_LIGHT_RED    = 101,
        BG_LIGHT_GREEN  = 102,
        BG_LIGHT_YELLOW = 103,
        BG_LIGHT_BLUE   = 104,
        BG_LIGHT_MAGENTA= 105,
        BG_LIGHT_CYAN   = 106,
        BG_WHITE        = 107,
        BG_DEFAULT      = 49,
        TEXT_DEFAULT    = 0,
        TEXT_BOLD       = 1,
        TEXT_NO_BOLD    = 21,
        TEXT_UNDERLINE  = 4,
        TEXT_NO_UNDERLINE= 24,
        TEXT_BLINK      = 5,
        TEXT_NO_BLINK   = 25,
        TEXT_REVERSE    = 7,
        TEXT_NO_REVERSE = 27,
        TEXT_INVISIBLE  = 8,
        TEXT_VISIBLE    = 28,
        TEXT_STRIKE     = 9,
        TEXT_NO_STRIKE  = 29
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
    class ExtendedModifier {
        unsigned int value;
        bool foreground;
    public:
        ExtendedModifier(unsigned int pValue, bool pForeground = true) : value(pValue), foreground(pForeground) {}
        friend std::ostream&
        operator<<(std::ostream& os, const ExtendedModifier& mod) {
            if(mod.value>256)return os;
            if(mod.foreground)return os << "\033[38;5;" << mod.value << "m";
            else return os << "\033[48;5;" << mod.value << "m";
        }
    };
}
/**
int main() {
    Color::Modifier red(Color::FG_RED);
    Color::Modifier def(Color::FG_DEFAULT);
    std::cout << "This ->" << red << "word" << def << "<- is red." << std::endl;
}
 */

#endif