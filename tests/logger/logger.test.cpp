#include <gtest/gtest.h>
// Purpose: Smoke-test the logger by capturing std::cout and ensuring messages
// are formatted and emitted. This does not validate timestamps/levels, only
// that messages appear in the output stream under the configured verbosity.
#include <gmock/gmock.h>
#include "logger/logger.h"
#include <iostream>
#include <sstream>
#include <string>

TEST(Logger, Output) {
    // Redirect std::cout to a string buffer for inspection.
    std::ostringstream output;
    std::streambuf* oldCoutBuffer = std::cout.rdbuf(output.rdbuf());
    {
        // Configure logger to emit to console with high verbosity and no file logging.
        CubeLog logger = CubeLog(0,Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS, Logger::LogLevel::LOGGER_DEBUG, Logger::LogLevel::LOGGER_OFF);
        logger.setConsoleLoggingEnabled(true);

        // Emit one message per level we care about.
        logger.info("This is an info message");
        logger.debug("This is a debug message");
        logger.warning("This is a warning message");
        logger.error("This is an error message");
        logger.warning("This is a warning message");

        // Restore std::cout so subsequent tests/user code are unaffected.
        std::cout.rdbuf(oldCoutBuffer);
    }
    // Assert that each expected message appears in the captured buffer.
    EXPECT_NE(output.str().find("This is an info message"), std::string::npos);
    EXPECT_NE(output.str().find("This is a debug message"), std::string::npos);
    EXPECT_NE(output.str().find("This is a warning message"), std::string::npos);
    EXPECT_NE(output.str().find("This is an error message"), std::string::npos);
    EXPECT_NE(output.str().find("This is a warning message"), std::string::npos);
}

TEST(Logger, LogToFile){
    // TODO: set up mocking for file logging so we can verify that file output
    // is produced without relying on the filesystem. For now, we disable
    // console logging and simply ensure calls do not crash.

    std::ostringstream output;
    std::streambuf* oldCoutBuffer = std::cout.rdbuf(output.rdbuf());
    {
        // Configure logger to emit to file (conceptually) with console disabled.
        CubeLog logger = CubeLog(0,Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS, Logger::LogLevel::LOGGER_OFF, Logger::LogLevel::LOGGER_DEBUG);
        logger.setConsoleLoggingEnabled(false);
        logger.info("This is an info message");
        logger.debug("This is a debug message");
        logger.warning("This is a warning message");
        logger.error("This is an error message");
        logger.warning("This is a warning message");

        // Restore std::cout to its original buffer.
        std::cout.rdbuf(oldCoutBuffer);
    }
    // TODO: When file logging is mockable, assert that the expected strings are
    // sent to the file sink. For now this is a non-crash smoke test.
}
