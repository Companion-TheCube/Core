#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>

class CUBE_LOG_ENTRY{
    private:
        std::string message;
        static int logEntryCount;
        std::chrono::time_point<std::chrono::system_clock> timestamp;
    public:
        CUBE_LOG_ENTRY(std::string message);
        std::string getMessage();
        std::string getTimestamp();
};

class CUBE_ERROR{
    private:
        std::string message;
        static int errorCount;
        std::chrono::time_point<std::chrono::system_clock> timestamp;
    public:
        CUBE_ERROR(std::string message);
        std::string getMessage();
        std::string getTimestamp();
};

class CubeLog{
    private:
        std::vector<CUBE_LOG_ENTRY> logEntries;
        std::vector<CUBE_ERROR> errors;
    public:
        void log(std::string message, bool print);
        void error(std::string message);
        std::vector<CUBE_LOG_ENTRY> getLogEntries();
        std::vector<CUBE_ERROR> getErrors();
        std::vector<std::string> getLogEntriesAsStrings();
        std::vector<std::string> getErrorsAsStrings();
        std::vector<std::string> getLogsAndErrorsAsStrings();
        CubeLog();
};

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp);