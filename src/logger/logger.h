#pragma once
#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <fstream>
#include <filesystem>

class ENTRY{
    public:
        virtual std::string getMessage() = 0;
        virtual std::string getTimestamp() = 0;
        virtual std::string getEntry() = 0;
};

class CUBE_LOG_ENTRY: public ENTRY{
    private:
        std::string message;
        static int logEntryCount;
        std::chrono::time_point<std::chrono::system_clock> timestamp;
    public:
        CUBE_LOG_ENTRY(std::string message);
        std::string getMessage();
        std::string getTimestamp();
        std::string getEntry();
};

class CUBE_ERROR: public ENTRY{
    private:
        std::string message;
        static int errorCount;
        std::chrono::time_point<std::chrono::system_clock> timestamp;
    public:
        CUBE_ERROR(std::string message);
        std::string getMessage();
        std::string getEntry();
        std::string getTimestamp();
};

class CubeLog{
    private:
        std::vector<CUBE_LOG_ENTRY> logEntries;
        std::vector<CUBE_ERROR> errors;
        std::mutex logMutex;
    public:
        void log(std::string message, bool print);
        void error(std::string message);
        std::vector<CUBE_LOG_ENTRY> getLogEntries();
        std::vector<CUBE_ERROR> getErrors();
        std::vector<std::string> getLogEntriesAsStrings();
        std::vector<std::string> getErrorsAsStrings();
        std::vector<std::string> getLogsAndErrorsAsStrings();
        CubeLog();
        void writeOutLogs();
};

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp);