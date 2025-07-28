/*
██╗      ██████╗  ██████╗  ██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗ 
██║     ██╔═══██╗██╔════╝ ██╔════╝ ██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██║     ██║   ██║██║  ███╗██║  ███╗█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██║     ██║   ██║██║   ██║██║   ██║██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝ 
███████╗╚██████╔╝╚██████╔╝╚██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║     
╚══════╝ ╚═════╝  ╚═════╝  ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// TODO: implement a way to check if a message is identical to the previous one and if so, print "repeated x times" on the previous message instead of th message

#include "logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#define COUNTER_MOD 1000 // determines how many dots we print while saving the logs
#define CUBE_LOG_ENTRY_MAX 100000 // maximum number of log entries in log file
#define LOG_WRITE_OUT_INTERVAL 300 // write out logs every 5 minutes
#define LOG_WRITE_OUT_COUNT 1500 // write out logs every 500 logs
#define CUBE_LOG_MEMORY_LIMIT 5000 // maximum number of log entries in memory

unsigned int CUBE_LOG_ENTRY::logEntryCount = 0;

/**
 * @brief Construct a new cube log_entry entry object
 *
 * @param message The message to log
 * @param location The source location of the log message
 * @param verbosity
 * @param level The log level of the message
 */
CUBE_LOG_ENTRY::CUBE_LOG_ENTRY(const std::string& message, CustomSourceLocation* location, Logger::LogVerbosity verbosity, Logger::LogLevel level)
{
    this->timestamp = std::chrono::system_clock::now();
    this->logEntryNumber = ++CUBE_LOG_ENTRY::logEntryCount;
    this->message = message;
    this->level = level;
    std::string fileName = getFileNameFromPath(location->file_name);
    switch (verbosity) {
    case Logger::LogVerbosity::MINIMUM:
        this->messageFull = message;
        break;
    case Logger::LogVerbosity::TIMESTAMP:
        this->messageFull = this->getTimestamp() + ": " + message;
        break;
    case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL:
        this->messageFull = this->getTimestamp() + ": " + message + " (" + Logger::logLevelStrings[(int)level] + ")";
        break;
    case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE:
        this->messageFull = this->getTimestamp() + ": " + fileName + ": " + message + " (" + Logger::logLevelStrings[(int)level] + ")";
        break;
    case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE:
        this->messageFull = this->getTimestamp() + ": " + fileName + "(" + std::to_string(location->line) + "): " + message + " (" + Logger::logLevelStrings[(int)level] + ")";
        break;
    case Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION:
        this->messageFull = this->getTimestamp() + ": " + fileName + "(" + std::to_string(location->line) + "): " + location->function_name + ": " + message + " (" + Logger::logLevelStrings[(int)level] + ")";
        break;
    default:
        this->messageFull = this->getTimestamp() + ": " + fileName + "(" + std::to_string(location->line) + "): " + location->function_name + ": " + message + " (" + std::to_string(CUBE_LOG_ENTRY::logEntryCount) + ")" + " (" + Logger::logLevelStrings[(int)level] + ")";
        break;
    }
}

/**
 * @brief Destroy the cube log_entry entry object
 *
 */
CUBE_LOG_ENTRY::~CUBE_LOG_ENTRY()
{
    // nothing to do
}

/**
 * @brief Get the log message as a string
 *
 * @return std::string
 */
std::string CUBE_LOG_ENTRY::getMessage()
{
    return this->message;
}

/**
 * @brief Get the log entry as a string with timestamp
 *
 * @return std::string
 */
std::string CUBE_LOG_ENTRY::getEntry()
{
    return this->getTimestamp() + ": " + this->getMessage();
}

/**
 * @brief Get the timestamp as a string
 *
 * @return std::string
 */
std::string CUBE_LOG_ENTRY::getTimestamp()
{
    return convertTimestampToString(this->timestamp);
}

/**
 * @brief Get the timestamp as a long
 *
 * @return unsigned long long The timestamp as a long
 */
unsigned long long CUBE_LOG_ENTRY::getTimestampAsLong()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(this->timestamp.time_since_epoch()).count();
}

/**
 * @brief Get the full log message
 *
 * @return std::string The full log message
 */
std::string CUBE_LOG_ENTRY::getMessageFull()
{
    return this->messageFull;
}

void CUBE_LOG_ENTRY::repeat(){
    repeatCount++;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

Logger::LogVerbosity CubeLog::staticVerbosity = Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS;
Logger::LogLevel CubeLog::staticPrintLevel = Logger::LogLevel::LOGGER_INFO;
std::vector<CUBE_LOG_ENTRY> CubeLog::logEntries;
std::mutex CubeLog::logMutex;
bool CubeLog::consoleLoggingEnabled = true;
bool CubeLog::hasUnreadErrors_b = false;
bool CubeLog::hasUnreadLogs_b = false;
std::vector<unsigned int> CubeLog::readErrorIDs;
std::vector<unsigned int> CubeLog::readLogIDs;
std::string CubeLog::screenMessage = "";
int CubeLog::advancedColorsEnabled = 0;
bool CubeLog::shutdown = false;
// spdlog file logger instance
std::shared_ptr<spdlog::logger> CubeLog::fileLogger = nullptr;

/**
 * @brief Log a message
 *
 * @param message The message to log
 * @param print If true, the message will be printed to the console
 * @param level The log level of the message
 * @param location The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::log(const std::string& message, bool print, Logger::LogLevel level, CustomSourceLocation location)
{
    if(CubeLog::shutdown) return;
    CUBE_LOG_ENTRY entry = CUBE_LOG_ENTRY(message, &location, CubeLog::staticVerbosity, level);
    if(CubeLog::logEntries.size() > 0 && CubeLog::logEntries.at(CubeLog::logEntries.size() - 1).getMessage() == entry.getMessage()){
        Color::Modifier blink(Color::TEXT_BLINK);
        Color::Modifier no_blink(Color::TEXT_NO_BLINK);
        CubeLog::logEntries.at(CubeLog::logEntries.size() - 1).repeat();
        if (CubeLog::logEntries.at(CubeLog::logEntries.size() - 1).getRepeatCount() == 1)
            std::cout << Color::Modifier(Color::TEXT_DEFAULT)
                      << "Previous log entry repeated "
                      << CubeLog::logEntries.at(CubeLog::logEntries.size() - 1).getRepeatCount()
                      << " times."
                      << Color::Modifier(Color::TEXT_DEFAULT)
                      << std::endl << std::flush;
        else
        {
            // Move cursor up and to line start, then reset colors
            std::cout << "\033[1A\033[G";
            std::cout << "\r";
            std::cout << Color::Modifier(Color::TEXT_DEFAULT)
                      << "Previous log entry repeated "
                      << CubeLog::logEntries.at(CubeLog::logEntries.size() - 1).getRepeatCount()
                      << " times.    "
                      << Color::Modifier(Color::TEXT_DEFAULT)
                      << std::endl << std::flush;
        }
        return;
    }
    CubeLog::logEntries.push_back(entry);
    Color::Modifier colorDebug(Color::FG_GREEN);
    Color::Modifier colorInfo(Color::FG_WHITE);
    Color::AdvancedModifier colorMoreInfo(200, 200, 200); // grey
    Color::AdvancedModifier colorWarning(50, 50, 255); // blue
    Color::Modifier colorError(Color::FG_LIGHT_YELLOW);
    Color::Modifier colorCritical(Color::FG_RED);
    Color::AdvancedModifier colorFatal(255, 50, 50); // bright red
    Color::Modifier colorDefault(Color::FG_LIGHT_BLUE);
    Color::AdvancedModifier colorDebugSilly(0, 255, 25); // brighter green
    if (CubeLog::advancedColorsEnabled == 2 && print && level >= CubeLog::staticPrintLevel && CubeLog::consoleLoggingEnabled && message.length() < 1000) {
        switch (level) {
#ifdef LOGGER_TRACE_ENABLED
        case Logger::LogLevel::LOGGER_TRACE:
            std::cout << colorDebugSilly << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
#endif
        case Logger::LogLevel::LOGGER_DEBUG_SILLY:
            std::cout << colorDebugSilly << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_DEBUG:
            std::cout << colorDebug << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_INFO:
            std::cout << colorInfo << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_WARNING:
            std::cout << colorWarning << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_ERROR:
            std::cout << colorError << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_CRITICAL:
            std::cout << colorCritical << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_FATAL:
            std::cout << colorFatal << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_MORE_INFO:
            std::cout << colorMoreInfo << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        case Logger::LogLevel::LOGGER_OFF:
            break;
        default:
            std::cout << colorDefault << entry.getMessageFull() << Color::Modifier(Color::TEXT_DEFAULT) << std::endl;
            break;
        }
    } else if (level != Logger::LogLevel::LOGGER_OFF && print && level >= CubeLog::staticPrintLevel && CubeLog::consoleLoggingEnabled && message.length() < 1000) {
        std::cout << entry.getMessageFull() << std::endl;
    }
    // file logging via spdlog
    if (CubeLog::fileLogger) {
        spdlog::level::level_enum spd_level = spdlog::level::info;
        switch (level) {
#ifdef LOGGER_TRACE_ENABLED
        case Logger::LogLevel::LOGGER_TRACE:
            spd_level = spdlog::level::trace;
            break;
#endif
        case Logger::LogLevel::LOGGER_DEBUG_SILLY:
        case Logger::LogLevel::LOGGER_DEBUG:
            spd_level = spdlog::level::debug;
            break;
        case Logger::LogLevel::LOGGER_INFO:
        case Logger::LogLevel::LOGGER_MORE_INFO:
            spd_level = spdlog::level::info;
            break;
        case Logger::LogLevel::LOGGER_WARNING:
            spd_level = spdlog::level::warn;
            break;
        case Logger::LogLevel::LOGGER_ERROR:
            spd_level = spdlog::level::err;
            break;
        case Logger::LogLevel::LOGGER_CRITICAL:
        case Logger::LogLevel::LOGGER_FATAL:
            spd_level = spdlog::level::critical;
            break;
        case Logger::LogLevel::LOGGER_OFF:
            spd_level = spdlog::level::off;
            break;
        default:
            spd_level = spdlog::level::info;
            break;
        }
        CubeLog::fileLogger->log(spd_level, entry.getMessageFull());
    }
}

std::chrono::system_clock::time_point CubeLog::lastScreenMessageTime = std::chrono::system_clock::now();

/**
 * @brief Log a message to the screen
 *
 * @param message The message to log
 * @param level The log level of the message
 * @param location The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::screen(const std::string& message, Logger::LogLevel level, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log("Screen Message: " + message, true, level, location);
    CubeLog::screenMessage = message;
    CubeLog::lastScreenMessageTime = std::chrono::system_clock::now();
}

/**
 * @brief Get the message destined for the screen
 *
 * @return std::string
 */
std::string CubeLog::getScreenMessage()
{
    std::unique_lock<std::mutex> lock(CubeLog::logMutex);
    return CubeLog::screenMessage;
}

/**
 * @brief Log a debug message
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::debugSilly(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_DEBUG_SILLY, location);
}

/**
 * @brief Log a debug message
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::debug(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_DEBUG, location);
}

/**
 * @brief Log an error message
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::error(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_ERROR, location);
}

/**
 * @brief Log an informational message
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::info(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_INFO, location);
}

/**
 * @brief Log a warning message
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::warning(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_WARNING, location);
}

/**
 * @brief Log a critical message, those where something failed and some functionality is now unavailable, but the application can still run.
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::critical(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_CRITICAL, location);
}

/**
 * @brief Log a "more info" message, those that are not errors, but provide additional information about the application state.
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::moreInfo(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_MORE_INFO, location);
}

/**
 * @brief Log a fatal message, those where the application cannot continue running and must exit.
 *
 * @param message The message to log
 * @param location *optional* The source location of the log message. If not provided, the location will be automatically determined.
 */
void CubeLog::fatal(const std::string& message, CustomSourceLocation location)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    CubeLog::log(message, true, Logger::LogLevel::LOGGER_FATAL, location);
}

/**
 * @brief Construct a new CubeLog object
 *
 * @param verbosity Determines the verbosity of the log messages.
 * @param printLevel Determines the level of log messages that will be printed to the console.
 * @param fileLevel Determines the level of log messages that will be written to the log file.
 */
CubeLog::CubeLog(int advancedColorsEnabled, Logger::LogVerbosity verbosity, Logger::LogLevel printLevel, Logger::LogLevel fileLevel)
{
    CubeLog::advancedColorsEnabled = advancedColorsEnabled;
    this->savingInProgress = false;
    CubeLog::staticVerbosity = verbosity;
    CubeLog::staticPrintLevel = printLevel;
    this->fileLevel = fileLevel;
    // initialize spdlog file logger if needed
    if (!CubeLog::fileLogger) {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs.txt", 1048576 * 5, 3);
        CubeLog::fileLogger = std::make_shared<spdlog::logger>("file_logger", sink);
        // set file logger level based on configured fileLevel
        spdlog::level::level_enum fl = spdlog::level::info;
        switch (fileLevel) {
#ifdef LOGGER_TRACE_ENABLED
        case Logger::LogLevel::LOGGER_TRACE:
            fl = spdlog::level::trace;    break;
#endif
        case Logger::LogLevel::LOGGER_DEBUG_SILLY:
        case Logger::LogLevel::LOGGER_DEBUG:
            fl = spdlog::level::debug;    break;
        case Logger::LogLevel::LOGGER_INFO:
        case Logger::LogLevel::LOGGER_MORE_INFO:
            fl = spdlog::level::info;     break;
        case Logger::LogLevel::LOGGER_WARNING:
            fl = spdlog::level::warn;     break;
        case Logger::LogLevel::LOGGER_ERROR:
            fl = spdlog::level::err;      break;
        case Logger::LogLevel::LOGGER_CRITICAL:
        case Logger::LogLevel::LOGGER_FATAL:
            fl = spdlog::level::critical; break;
        case Logger::LogLevel::LOGGER_OFF:
            fl = spdlog::level::off;      break;
        default:
            fl = spdlog::level::info;     break;
        }
        CubeLog::fileLogger->set_level(fl);
        // flush on the configured file logging level to ensure debug_silly logs are flushed promptly
        CubeLog::fileLogger->flush_on(fl);
    }
    // this->saveLogsThread = std::jthread([this] {
    //     this->saveLogsInterval();
    // });
    CubeLog::log("Logger initialized", true);
    // create a signal to the thread that will kill the loop when we exit

    this->resetThread = new std::jthread([&](std::stop_token st) {
        while (!st.stop_requested()) {
            // get current time point
            auto end = std::chrono::system_clock::now();
            std::unique_lock<std::mutex> lock(CubeLog::logMutex);
            // get the duration
            std::chrono::duration<double> duration = end.time_since_epoch() - CubeLog::lastScreenMessageTime.time_since_epoch();
            // if the duration is greater than 3 seconds, clear the screen message
            if (duration.count() > 3 && CubeLog::screenMessage.length() > 0) {
                CubeLog::screen("");
            }
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (st.stop_requested())
                break;
        }
    });
}

/**
 * @brief Save logs to file every LOG_WRITE_OUT_INTERVAL seconds
 *
 */
void CubeLog::saveLogsInterval()
{
    // TODO: modify this to use std::stop_token
    while (true) {
        for (size_t i = 0; i < LOG_WRITE_OUT_INTERVAL; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // if (this->logEntries.size() > LOG_WRITE_OUT_COUNT + this->savedLogsCount) {
            //     // this->writeOutLogs();
            // }
            std::lock_guard<std::mutex> lock(this->saveLogsThreadRunMutex);
            if (!saveLogsThreadRun)
                break;
        }
        {
            std::lock_guard<std::mutex> lock(this->saveLogsThreadRunMutex);
            if (!saveLogsThreadRun)
                break;
        }
        // this->writeOutLogs();
        this->purgeOldLogs();
        this->savedLogsCount = 0;
    }
}

/**
 * @brief Purge old logs from memory. The number of logs in memory will be limited to CUBE_LOG_MEMORY_LIMIT.
 *
 */
void CubeLog::purgeOldLogs()
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    if (this->logEntries.size() > CUBE_LOG_MEMORY_LIMIT) {
        this->logEntries.erase(this->logEntries.begin(), this->logEntries.end() - CUBE_LOG_MEMORY_LIMIT);
    }
    this->logEntries.shrink_to_fit(); // TODO: this may not be needed since the log is just going to grow again
}

/**
 * @brief Destroy the CubeLog object
 *
 */
CubeLog::~CubeLog()
{
    CubeLog::info("Logger shutting down");
    // this->writeOutLogs();
    CubeLog::shutdown = true;
    resetThread->request_stop();
    if(resetThread->joinable())
        resetThread->join();
    std::lock_guard<std::mutex> lock(this->saveLogsThreadRunMutex);
    this->saveLogsThreadRun = false;
    Color::Modifier fgColorReset(Color::FG_DEFAULT);
    Color::Modifier bgColorReset(Color::BG_DEFAULT);
    if (this->consoleLoggingEnabled)
        std::cout << fgColorReset << bgColorReset << std::endl;
}

/**
 * @brief Get the log entries
 *
 * @param level The log level to get. If not provided, all log entries will be returned.
 * @return std::vector<CUBE_LOG_ENTRY> The log entries
 */
std::vector<CUBE_LOG_ENTRY> CubeLog::getLogEntries(Logger::LogLevel level)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    std::vector<CUBE_LOG_ENTRY> logEntries;
    for (size_t i = 0; i < this->logEntries.size(); i++) {
        if (this->logEntries[i].level >= level)
            logEntries.push_back(this->logEntries[i]);
    }
    return logEntries;
}

/**
 * @brief Get the log entries as strings
 *
 * @param fullMessages If true, the full log messages will be returned. If false, only the message will be returned.
 * @return std::vector<std::string> The log entries as strings
 */
std::vector<std::string> CubeLog::getLogEntriesAsStrings(bool fullMessages)
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    std::vector<std::string> logEntriesAsStrings;
    for (size_t i = 0; i < this->logEntries.size(); i++) {
        if (fullMessages)
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
std::vector<std::string> CubeLog::getLogsAndErrorsAsStrings(bool fullMessages)
{
    // time how long this function takes
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::string> logsAndErrorsAsStrings;
    std::vector<CUBE_LOG_ENTRY> logEntries = this->getLogEntries(this->fileLevel);
    std::vector<std::pair<unsigned long long, std::string>> logsAndErrorsAsPairs;
    for (auto entry : logEntries) {
        logsAndErrorsAsPairs.push_back({ entry.getTimestampAsLong(), entry.getMessageFull() });
    }
    std::stable_sort(logsAndErrorsAsPairs.begin(), logsAndErrorsAsPairs.end());
    for (auto pair : logsAndErrorsAsPairs) {
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
void CubeLog::writeOutLogs()
{
    // TODO: theres a bug here where the logs are being written to the file and existing logs are being overwritten. FIXME
    if (this->fileLevel == Logger::LogLevel::LOGGER_OFF)
        return;
    std::cout << "Size of CubeLog: " << CubeLog::getSizeOfCubeLog() << std::endl;
    std::cout << "Memory footprint: " << getMemoryFootprint() << std::endl;
    std::lock_guard<std::mutex> lock(this->saveLogsMutex);
    if (this->savingInProgress)
        return;
    this->savingInProgress = true;
    std::cout << "Writing logs to file..." << std::endl;
    std::vector<std::string> logsAndErrors = this->getLogsAndErrorsAsStrings();
    // write to file
    this->savedLogsCount = logsAndErrors.size();
    std::filesystem::path p("logs.txt");
    if (!std::filesystem::exists(p)) {
        std::ofstream file("logs.txt");
        file.close();
    }
    std::ifstream file("logs.txt");
    std::string line;
    std::vector<std::string> existingLogs;
    size_t counter = 0;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            existingLogs.push_back(line);
            counter++;
            if (counter % COUNTER_MOD == 0)
                std::cout << ".";
        }
        file.close();
    }
    std::ofstream outFile("logs.txt"); // overwrites the file
    // get count of existing logs
    long long existingLogsCount = existingLogs.size();
    // add to count of new logs
    long long newLogsCount = logsAndErrors.size();
    // if the total number of logs is greater than CUBE_LOG_ENTRY_MAX, remove the oldest logs
    if (existingLogsCount + newLogsCount > CUBE_LOG_ENTRY_MAX) {
        long long numToRemove = existingLogsCount + newLogsCount - CUBE_LOG_ENTRY_MAX;
        if (numToRemove > existingLogsCount) {
            existingLogs.clear();
            numToRemove = 0;
        } else {
            existingLogs.erase(existingLogs.begin(), existingLogs.begin() + numToRemove);
        }
    }
    // write the existing logs to the file
    counter = 0;
    for (; counter < existingLogs.size(); counter++) {
        outFile << existingLogs[counter] << std::endl;
        if (counter % COUNTER_MOD == 0)
            std::cout << ".";
    }
    // find the first log entry in logsAndErrors that is not in existingLogs
    counter = 0;
    while (counter < logsAndErrors.size() && std::find(existingLogs.begin(), existingLogs.end(), logsAndErrors.at(counter)) != existingLogs.end()) {
        counter++;
        if (counter % COUNTER_MOD == 0)
            std::cout << ".";
    }
    // write the new logs to the file
    for (; counter < logsAndErrors.size(); counter++) {
        outFile << logsAndErrors[counter] << std::endl;
        if (counter % COUNTER_MOD == 0)
            std::cout << ".";
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
void CubeLog::setVerbosity(Logger::LogVerbosity verbosity)
{
    this->log("Setting verbosity to " + std::to_string((int)verbosity), true);
    CubeLog::staticVerbosity = verbosity;
}

/**
 * @brief Convert a timestamp to a string
 *
 * @param timestamp The timestamp to convert
 * @return std::string The timestamp as a string
 */
std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp)
{
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
std::string getFileNameFromPath(const std::string& path)
{
    std::filesystem::path p(path);
    return p.filename().string();
}

/**
 * @brief Set the log level for printing to the console and writing to the log file
 *
 * @param printLevel The log level for printing to the console
 * @param fileLevel The log level for writing to the log file
 */
void CubeLog::setLogLevel(Logger::LogLevel printLevel, Logger::LogLevel fileLevel)
{
    CubeLog::staticPrintLevel = printLevel;
    this->fileLevel = fileLevel;
    // update the spdlog file logger level and flush threshold if it's already initialized
    if (CubeLog::fileLogger) {
        spdlog::level::level_enum fl = spdlog::level::info;
        switch (fileLevel) {
#ifdef LOGGER_TRACE_ENABLED
        case Logger::LogLevel::LOGGER_TRACE:
            fl = spdlog::level::trace; break;
#endif
        case Logger::LogLevel::LOGGER_DEBUG_SILLY:
        case Logger::LogLevel::LOGGER_DEBUG:
            fl = spdlog::level::debug; break;
        case Logger::LogLevel::LOGGER_INFO:
        case Logger::LogLevel::LOGGER_MORE_INFO:
            fl = spdlog::level::info; break;
        case Logger::LogLevel::LOGGER_WARNING:
            fl = spdlog::level::warn; break;
        case Logger::LogLevel::LOGGER_ERROR:
            fl = spdlog::level::err; break;
        case Logger::LogLevel::LOGGER_CRITICAL:
        case Logger::LogLevel::LOGGER_FATAL:
            fl = spdlog::level::critical; break;
        case Logger::LogLevel::LOGGER_OFF:
            fl = spdlog::level::off; break;
        default:
            fl = spdlog::level::info; break;
        }
        CubeLog::fileLogger->set_level(fl);
        CubeLog::fileLogger->flush_on(fl);
    }
}

/**
 * @brief Set whether logging to the console is enabled
 *
 * @param enabled If true, logging to the console is enabled
 */
void CubeLog::setConsoleLoggingEnabled(bool enabled)
{
    CubeLog::consoleLoggingEnabled = enabled;
}

/**
 * @brief Get the latest error
 *
 * @return CUBE_LOG_ENTRY The latest error
 */
CUBE_LOG_ENTRY CubeLog::getLatestError()
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    // find the latest error entry that is not in the readErrorIDs vector
    for (size_t i = CubeLog::logEntries.size() - 1; i >= 0; i--) {
        if (CubeLog::logEntries[i].level == Logger::LogLevel::LOGGER_ERROR && std::find(CubeLog::readErrorIDs.begin(), CubeLog::readErrorIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readErrorIDs.end()) {
            CubeLog::readErrorIDs.push_back(CubeLog::logEntries[i].logEntryNumber);
            return CubeLog::logEntries[i];
        }
    }
    return CUBE_LOG_ENTRY("No errors found", nullptr, Logger::LogVerbosity::MINIMUM, Logger::LogLevel::LOGGER_INFO);
}

/**
 * @brief Get the latest log
 *
 * @return CUBE_LOG_ENTRY The latest log
 */
CUBE_LOG_ENTRY CubeLog::getLatestLog()
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    // find the latest log entry that is not an error and is not in the readLogIDs vector
    for (size_t i = CubeLog::logEntries.size() - 1; i >= 0; i--) {
        if (CubeLog::logEntries[i].level != Logger::LogLevel::LOGGER_ERROR && std::find(CubeLog::readLogIDs.begin(), CubeLog::readLogIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readLogIDs.end()) {
            CubeLog::readLogIDs.push_back(CubeLog::logEntries[i].logEntryNumber);
            return CubeLog::logEntries[i];
        }
    }
    return CUBE_LOG_ENTRY("No logs found", nullptr, Logger::LogVerbosity::MINIMUM, Logger::LogLevel::LOGGER_INFO);
}

/**
 * @brief Get the log latest entry
 */
CUBE_LOG_ENTRY CubeLog::getLatestEntry()
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    if (CubeLog::logEntries.size() > 0) {
        return CubeLog::logEntries[CubeLog::logEntries.size() - 1];
        // add to readLogIDs or readErrorIDs depending on the level
        if (CubeLog::logEntries[CubeLog::logEntries.size() - 1].level == Logger::LogLevel::LOGGER_ERROR) {
            CubeLog::readErrorIDs.push_back(CubeLog::logEntries[CubeLog::logEntries.size() - 1].logEntryNumber);
        } else {
            CubeLog::readLogIDs.push_back(CubeLog::logEntries[CubeLog::logEntries.size() - 1].logEntryNumber);
        }
    }
    return CUBE_LOG_ENTRY("No logs found", nullptr, Logger::LogVerbosity::MINIMUM, Logger::LogLevel::LOGGER_INFO);
}

bool CubeLog::hasUnreadErrors()
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    bool hasUnreadErrors = false;
    // determine if any of the errors in the logEntries vector that are Logger::LogLevel::LOGGER_ERROR are not in the readErrorIDs vector
    for (size_t i = CubeLog::logEntries.size() - 1; i >= 0; i--) {
        if (CubeLog::logEntries[i].level == Logger::LogLevel::LOGGER_ERROR && std::find(CubeLog::readErrorIDs.begin(), CubeLog::readErrorIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readErrorIDs.end()) {
            hasUnreadErrors = true;
            break;
        }
    }
    return hasUnreadErrors;
}

bool CubeLog::hasUnreadLogs()
{
    std::lock_guard<std::mutex> lock(CubeLog::logMutex);
    bool hasUnreadLogs = false;
    // determine if any of the logs in the logEntries vector that are not Logger::LogLevel::LOGGER_ERROR are not in the readLogIDs vector
    for (size_t i = CubeLog::logEntries.size() - 1; i >= 0; i--) {
        if (CubeLog::logEntries[i].level != Logger::LogLevel::LOGGER_ERROR && std::find(CubeLog::readLogIDs.begin(), CubeLog::readLogIDs.end(), CubeLog::logEntries[i].logEntryNumber) == CubeLog::readLogIDs.end()) {
            hasUnreadLogs = true;
            break;
        }
    }
    return hasUnreadLogs;
}

bool CubeLog::hasUnreadEntries()
{
    return CubeLog::hasUnreadErrors() || CubeLog::hasUnreadLogs();
}

/**
 * @brief Get the name of the interface
 *
 * @return std::string The name of the interface
 */
constexpr std::string CubeLog::getInterfaceName() const
{
    return "Logger";
}

const std::string sanitizeString(std::string str)
{
    // iterate through the string and ensure utf-8 compliance
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] < 0) {
            str[i] = '?';
        }
    }
    return str;
}

/**
 * @brief Get the endpoint data
 *
 * @return HttpEndPointData_t The endpoint data
 */
// TODO: add endpoint(s) to get logs. perhaps have the ability to get logs by level, by date, etc. and/or logs from memory or from file
HttpEndPointData_t CubeLog::getHttpEndpointData()
{
    HttpEndPointData_t data;
    data.push_back({ PUBLIC_ENDPOINT | POST_ENDPOINT,
        [](const httplib::Request& req, httplib::Response& res) {
            // log info
            CubeLog::info("Logging message from endpoint");
            if ((req.has_header("Content-Type") && req.get_header_value("Content-Type") == "application/json")) {
                std::string message;
                std::string level;
                std::string source;
                std::string line;
                std::string function;
                try {
                    nlohmann::json received = nlohmann::json::parse(req.body);
                    message = received["message"];
                    level = received["level"];
                    source = received["source"];
                    line = received["line"];
                    function = received["function"];
                } catch (nlohmann::json::exception e) {
                    CubeLog::error(e.what());
                    // create json object with error message and return success = false
                    nlohmann::json j;
                    j["success"] = false;
                    j["message"] = e.what();
                    res.set_content(j.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
                }
                // convert level to int
                int level_int;
                try {
                    level_int = std::stoi(level);
                    if (level_int < 0 || level_int > (int)Logger::LogLevel::LOGGER_LOGLEVELCOUNT - 1)
                        throw std::invalid_argument("Log level out of range. Must be between 0 and " + std::to_string((int)Logger::LogLevel::LOGGER_LOGLEVELCOUNT - 1) + ", inclusive.");
                } catch (std::invalid_argument e) {
                    CubeLog::error(e.what());
                    // create json object with error message and return success = false
                    nlohmann::json j;
                    j["success"] = false;
                    j["message"] = e.what();
                    res.set_content(j.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, e.what());
                }
                // TODO: source string should be prepended with the name of the source app or it's IP or something. That way, we know
                // for sure where the log message is coming from. Without this, an app can log stuff while pretending to be another app.
                auto location = CustomSourceLocation(source.c_str(), std::stoi(line), 0, function.c_str());
                CubeLog::log(message, true, Logger::LogLevel(level_int), location); // We use CubeLog::log here so we can set the level
                nlohmann::json j;
                j["success"] = true;
                j["message"] = "Logged message";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Logged message");
            } else {
                CubeLog::error("Content-Type header must be set to \"application/json\".");
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "Content-Type header must be set to \"application/json\".";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Content-Type header must be set to \"application/json\".");
            }
        },
        "log",
        { "message", "level", "source", "line", "function" },
        "Log a message" });
    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            CUBELOG_TRACE("Getting logs for endpoint");
            std::vector<CUBE_LOG_ENTRY> entries = CubeLog::getLogEntries();
            nlohmann::json j;
            j["entries"] = nlohmann::json::array();
            for (auto entry : entries) {
                nlohmann::json entryJson;
                entryJson["timestamp"] = sanitizeString(entry.getTimestamp());
                entryJson["message"] = sanitizeString(entry.getMessageFull());
                entryJson["level"] = sanitizeString(Logger::logLevelStrings[(int)entry.level]);
                j["entries"].push_back(entryJson);
            }
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Got logs");
        },
        "getLogs",
        {},
        "Get logs" });
    return data;
}

/**
 * @brief Get the endpoint names
 *
 * @return std::vector<std::string> The endpoint names
 */
// std::vector<std::pair<std::string, std::vector<std::string>>> CubeLog::getHttpEndpointNamesAndParams()
// {
//     std::vector<std::pair<std::string, std::vector<std::string>>> names;
//     names.push_back({ "log", { "message", "level", "source", "line", "function" } });
//     names.push_back({ "getLogs", {} });
//     return names;
// }

/**
 * @brief Get the size of the CubeLog in memory
 *
 * @return std::string The size of the CubeLog
 */
std::string CubeLog::getSizeOfCubeLog()
{
    size_t num_entries = CubeLog::logEntries.size();
    size_t total_size = 0;
    for (auto entry : CubeLog::logEntries) {
        total_size += sizeof(entry) + entry.getMessage().size() + entry.getMessageFull().size();
    }
    return std::to_string(num_entries) + " entries, " + std::to_string(total_size) + " bytes";
}
