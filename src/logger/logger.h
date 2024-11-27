#ifndef LOGGER_H
#define LOGGER_H
#define WIN32_LEAN_AND_MEAN
#ifndef API_H
#include "../api/api.h"
#endif
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <source_location>
#include <sstream>
#include <utils.h>
#include <vector>

#define LOGGER_TRACE_ENABLED
#ifdef LOGGER_TRACE_ENABLED
#define CUBELOG_TRACE(x) CubeLog::log(x, true, Logger::LogLevel::LOGGER_TRACE, CustomSourceLocation::current())
#else
#define CUBELOG_TRACE(x)
#endif

namespace Logger {
enum class LogVerbosity : int {
    MINIMUM,
    TIMESTAMP,
    TIMESTAMP_AND_LEVEL,
    TIMESTAMP_AND_LEVEL_AND_FILE,
    TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE,
    TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION,
    TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS,
    LOGVERBOSITYCOUNT
};

enum class LogLevel : int {
#ifdef LOGGER_TRACE_ENABLED
    LOGGER_TRACE,
#endif
    LOGGER_DEBUG_SILLY,
    LOGGER_DEBUG,
    LOGGER_INFO,
    LOGGER_MORE_INFO,
    LOGGER_WARNING,
    LOGGER_ERROR,
    LOGGER_CRITICAL,
    LOGGER_FATAL,
    LOGGER_OFF,
    LOGGER_LOGLEVELCOUNT
};

const std::string logLevelStrings[] = {
#ifdef LOGGER_TRACE_ENABLED
    "TRACE",
#endif
    "DEBUG_SILLY",
    "DEBUG",
    "INFO",
    "MORE_INFO",
    "WARNING",
    "ERROR",
    "CRITICAL",
    "FATAL",
    "OFF",
    "LOGLEVELCOUNT"
};

} // namespace Logger

struct CustomSourceLocation {
    const char* file_name;
    std::uint_least32_t line;
    std::uint_least32_t column;
    const char* function_name;

    CustomSourceLocation(const char* file, std::uint_least32_t line_num, std::uint_least32_t col_num, const char* func)
        : file_name(file)
        , line(line_num)
        , column(col_num)
        , function_name(func)
    {
    }

    static CustomSourceLocation current(const std::source_location& loc = std::source_location::current())
    {
        return CustomSourceLocation(loc.file_name(), loc.line(), loc.column(), loc.function_name());
    }
};

class CUBE_LOG_ENTRY {
private:
    std::string message;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string messageFull;

public:
    unsigned int logEntryNumber;
    Logger::LogLevel level;
    static unsigned int logEntryCount;
    CUBE_LOG_ENTRY(const std::string& message, CustomSourceLocation* location, Logger::LogVerbosity verbosity, Logger::LogLevel level = Logger::LogLevel::LOGGER_INFO);
    ~CUBE_LOG_ENTRY();
    std::string getMessage();
    std::string getTimestamp();
    std::string getEntry();
    std::string getMessageFull();
    unsigned long long getTimestampAsLong();
};

class CubeLog : public AutoRegisterAPI<CubeLog> {
private:
    static std::vector<CUBE_LOG_ENTRY> logEntries;
    static std::mutex logMutex;
    static Logger::LogVerbosity staticVerbosity;
    static Logger::LogLevel staticPrintLevel;
    static bool consoleLoggingEnabled;
    static bool shutdown;
    Logger::LogLevel fileLevel;
    bool savingInProgress;
    void saveLogsInterval();
    std::jthread saveLogsThread;
    std::mutex saveLogsMutex;
    unsigned long long savedLogsCount = 0;
    bool saveLogsThreadRun = true;
    std::mutex saveLogsThreadRunMutex;
    void purgeOldLogs();
    static bool hasUnreadErrors_b, hasUnreadLogs_b;
    static std::string screenMessage;
    static std::vector<unsigned int> readErrorIDs, readLogIDs;
    static void log(const std::string& message, bool print, Logger::LogLevel level = Logger::LogLevel::LOGGER_INFO, CustomSourceLocation location = CustomSourceLocation::current());
    std::jthread* resetThread;
    static std::chrono::system_clock::time_point lastScreenMessageTime;
    static int advancedColorsEnabled;

public:
    static void screen(const std::string& message, Logger::LogLevel level = Logger::LogLevel::LOGGER_INFO, CustomSourceLocation location = CustomSourceLocation::current());
    static void debugSilly(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    static void debug(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    static void error(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    static void info(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    static void warning(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    static void critical(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    static void moreInfo(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    static void fatal(const std::string& message, CustomSourceLocation location = CustomSourceLocation::current());
    std::vector<CUBE_LOG_ENTRY> getLogEntries(Logger::LogLevel level = Logger::LogLevel(0));
    std::vector<std::string> getLogEntriesAsStrings(bool fullMessages = true);
    std::vector<std::string> getErrorsAsStrings(bool fullMessages = true);
    std::vector<std::string> getLogsAndErrorsAsStrings(bool fullMessages = true);
    CubeLog(int advancedColorsEnabled = 2, Logger::LogVerbosity verbosity = Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION, Logger::LogLevel printLevel = Logger::LogLevel::LOGGER_INFO, Logger::LogLevel fileLevel = Logger::LogLevel::LOGGER_INFO);
    ~CubeLog();
    void writeOutLogs();
    void setVerbosity(Logger::LogVerbosity verbosity);
    void setLogLevel(Logger::LogLevel printLevel, Logger::LogLevel fileLevel);
    static void setConsoleLoggingEnabled(bool enabled);
    static CUBE_LOG_ENTRY getLatestError();
    static CUBE_LOG_ENTRY getLatestLog();
    static CUBE_LOG_ENTRY getLatestEntry();
    static bool hasUnreadErrors();
    static bool hasUnreadLogs();
    static bool hasUnreadEntries();
    static std::string getScreenMessage();
    static std::string getSizeOfCubeLog();
    // API Interface
    constexpr std::string getInterfaceName() const override;
    HttpEndPointData_t getHttpEndpointData();
};

std::string convertTimestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp);
std::string getFileNameFromPath(const std::string& path);

namespace Color {
enum Code {
    FG_RED = 31,
    FG_GREEN = 32,
    FG_BLUE = 34,
    FG_MAGENTA = 35,
    FG_CYAN = 36,
    FG_LIGHT_GRAY = 37,
    FG_YELLOW = 33,
    FG_DARK_GRAY = 90,
    FG_LIGHT_RED = 91,
    FG_LIGHT_GREEN = 92,
    FG_LIGHT_YELLOW = 93,
    FG_LIGHT_BLUE = 94,
    FG_LIGHT_MAGENTA = 95,
    FG_LIGHT_CYAN = 96,
    FG_WHITE = 97,
    FG_DEFAULT = 39,
    BG_RED = 41,
    BG_GREEN = 42,
    BG_BLUE = 44,
    BG_MAGENTA = 45,
    BG_CYAN = 46,
    BG_LIGHT_GRAY = 47,
    BG_DARK_GRAY = 100,
    BG_LIGHT_RED = 101,
    BG_LIGHT_GREEN = 102,
    BG_LIGHT_YELLOW = 103,
    BG_LIGHT_BLUE = 104,
    BG_LIGHT_MAGENTA = 105,
    BG_LIGHT_CYAN = 106,
    BG_WHITE = 107,
    BG_DEFAULT = 49,
    TEXT_DEFAULT = 0,
    TEXT_BOLD = 1,
    TEXT_NO_BOLD = 21,
    TEXT_UNDERLINE = 4,
    TEXT_NO_UNDERLINE = 24,
    TEXT_BLINK = 5,
    TEXT_NO_BLINK = 25,
    TEXT_REVERSE = 7,
    TEXT_NO_REVERSE = 27,
    TEXT_INVISIBLE = 8,
    TEXT_VISIBLE = 28,
    TEXT_STRIKE = 9,
    TEXT_NO_STRIKE = 29
};

class Modifier {
    Code code;

public:
    Modifier(Code pCode)
        : code(pCode)
    {
    }
    friend std::ostream&
    operator<<(std::ostream& os, const Modifier& mod)
    {
        return os << "\033[" << mod.code << "m";
    }
};

class ExtendedModifier {
    unsigned int value;
    bool foreground;

public:
    ExtendedModifier(unsigned int pValue, bool pForeground = true)
        : value(pValue)
        , foreground(pForeground)
    {
    }
    friend std::ostream&
    operator<<(std::ostream& os, const ExtendedModifier& mod)
    {
        if (mod.value > 256)
            return os;
        if (mod.foreground)
            return os << "\033[38;5;" << mod.value << "m";
        else
            return os << "\033[48;5;" << mod.value << "m";
    }
};

class AdvancedModifier {
    unsigned int r, g, b;
    bool foreground;

public:
    AdvancedModifier(unsigned int pR, unsigned int pG, unsigned int pB, bool pForeground = true)
        : r(pR)
        , g(pG)
        , b(pB)
        , foreground(pForeground)
    {
    }
    friend std::ostream&
    operator<<(std::ostream& os, const AdvancedModifier& mod)
    {
        if (mod.r > 255 || mod.g > 255 || mod.b > 255)
            return os;
        if (mod.foreground)
            return os << "\033[38;2;" << mod.r << ";" << mod.g << ";" << mod.b << "m";
        else
            return os << "\033[48;2;" << mod.r << ";" << mod.g << ";" << mod.b << "m";
    }
};
} // namespace Color

#endif // LOGGER_H