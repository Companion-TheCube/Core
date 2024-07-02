#pragma once
#include <logger.h>
#include <string>
#include <filesystem>
#include <iostream>
#ifdef __linux__
#include <dirent.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif
#include <fstream>
#include <string>
#include "RunningApp.h"

class NativeAPI {
public:
    static RunningApp* startApp(std::string execPath, std::string execArgs, std::string appID, std::string appName, std::string appSource, std::string updatePath);
    static bool stopApp(std::string execPath);
    static bool stopApp(long pid);
    static long getPID(std::string execPath);
    static bool isProcessRunning(const std::string& processName);
};