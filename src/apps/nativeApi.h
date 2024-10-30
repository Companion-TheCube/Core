#pragma once
#include <filesystem>
#include <iostream>
#include <logger.h>
#include <string>
#ifdef __linux__
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <tlhelp32.h>
#endif
#include "RunningApp.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <utils.h>

class NativeAPI {
public:
    static RunningApp* startApp(const std::string& execPath, const std::string& execArgs, const std::string& appID, const std::string& appName, const std::string& appSource, const std::string& updatePath);
    static bool stopApp(const std::string& execPath);
    static bool stopApp(long pid);
    static long getPID(const std::string& execPath);
    static bool isProcessRunning(const std::string& processName);
    static bool isProcessRunning(long pid);
    static bool isExecutableInstalled(const std::string& execPath);
};