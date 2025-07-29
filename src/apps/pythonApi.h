/*
██████╗ ██╗   ██╗████████╗██╗  ██╗ ██████╗ ███╗   ██╗ █████╗ ██████╗ ██╗   ██╗  ██╗
██╔══██╗╚██╗ ██╔╝╚══██╔══╝██║  ██║██╔═══██╗████╗  ██║██╔══██╗██╔══██╗██║   ██║  ██║
██████╔╝ ╚████╔╝    ██║   ███████║██║   ██║██╔██╗ ██║███████║██████╔╝██║   ███████║
██╔═══╝   ╚██╔╝     ██║   ██╔══██║██║   ██║██║╚██╗██║██╔══██║██╔═══╝ ██║   ██╔══██║
██║        ██║      ██║   ██║  ██║╚██████╔╝██║ ╚████║██║  ██║██║     ██║██╗██║  ██║
╚═╝        ╚═╝      ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝╚═╝  ╚═╝
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