#pragma once
#include <filesystem>
#include <iostream>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include <string>
#ifdef __linux__
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "RunningApp.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <utils.h>

#ifndef __linux__
static_assert(false, "This code is only for Linux.");
#endif

class PythonAPI {
public:
    static std::unique_ptr<RunningApp> startApp(const std::string& execPath, const std::string& execArgs, const std::string& appID, const std::string& appName, const std::string& appSource, const std::string& updatePath);
    static bool stopApp(long pid);
    static long getPID(const std::string& execPath, const std::string& execArgs);
    static bool isProcessRunning(long pid);
    static bool isProcessRunning(const std::string& scriptName, const std::string& execArgs);
};