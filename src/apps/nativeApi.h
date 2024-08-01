#pragma once
#include <logger.h>
#include <string>
#include <filesystem>
#include <iostream>
#ifdef __linux__
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <cstdlib>
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
#include <fstream>
#include <string>
#include "RunningApp.h"
#include <utils.h>
#include <algorithm>

class NativeAPI {
public:
    static RunningApp* startApp(std::string execPath, std::string execArgs, std::string appID, std::string appName, std::string appSource, std::string updatePath);
    static bool stopApp(std::string execPath);
    static bool stopApp(long pid);
    static long getPID(std::string execPath);
    static bool isProcessRunning(const std::string& processName);
    static bool isProcessRunning(long pid);
    static bool isExecutableInstalled(std::string execPath);
};