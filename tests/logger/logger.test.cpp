#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/logger/logger.cpp"
#include <iostream>
#include <sstream>
#include <string>

TEST(Logger, Output) {
    std::ostringstream output;
    std::streambuf* oldCoutBuffer = std::cout.rdbuf(output.rdbuf());
    {
        // Call the function that prints to stdout
        CubeLog logger = CubeLog(0,Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS, Logger::LogLevel::LOGGER_DEBUG, Logger::LogLevel::LOGGER_OFF);
        logger.setConsoleLoggingEnabled(true);
        logger.info("This is an info message");
        logger.debug("This is a debug message");
        logger.warning("This is a warning message");
        logger.error("This is an error message");
        logger.warning("This is a warning message");

        // Restore the original stdout buffer
        std::cout.rdbuf(oldCoutBuffer);
    }
    // Check if the output matches the expected result
    EXPECT_NE(output.str().find("This is an info message"), std::string::npos);
    EXPECT_NE(output.str().find("This is a debug message"), std::string::npos);
    EXPECT_NE(output.str().find("This is a warning message"), std::string::npos);
    EXPECT_NE(output.str().find("This is an error message"), std::string::npos);
    EXPECT_NE(output.str().find("This is a warning message"), std::string::npos);
}

TEST(Logger, LogToFile){
    // TODO: set up mocking for file logging

    std::ostringstream output;
    std::streambuf* oldCoutBuffer = std::cout.rdbuf(output.rdbuf());
    {
        // Call the function that prints to stdout
        CubeLog logger = CubeLog(0,Logger::LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS, Logger::LogLevel::LOGGER_OFF, Logger::LogLevel::LOGGER_DEBUG);
        logger.setConsoleLoggingEnabled(false);
        logger.info("This is an info message");
        logger.debug("This is a debug message");
        logger.warning("This is a warning message");
        logger.error("This is an error message");
        logger.warning("This is a warning message");

        // Restore the original stdout buffer
        std::cout.rdbuf(oldCoutBuffer);
    }
    // TODO: Check if the output matches the expected result    
}