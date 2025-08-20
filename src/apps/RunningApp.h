/*
██████╗ ██╗   ██╗███╗   ██╗███╗   ██╗██╗███╗   ██╗ ██████╗  █████╗ ██████╗ ██████╗ ██╗  ██╗
██╔══██╗██║   ██║████╗  ██║████╗  ██║██║████╗  ██║██╔════╝ ██╔══██╗██╔══██╗██╔══██╗██║  ██║
██████╔╝██║   ██║██╔██╗ ██║██╔██╗ ██║██║██╔██╗ ██║██║  ███╗███████║██████╔╝██████╔╝███████║
██╔══██╗██║   ██║██║╚██╗██║██║╚██╗██║██║██║╚██╗██║██║   ██║██╔══██║██╔═══╝ ██╔═══╝ ██╔══██║
██║  ██║╚██████╔╝██║ ╚████║██║ ╚████║██║██║ ╚████║╚██████╔╝██║  ██║██║     ██║  ██╗██║  ██║
╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═══╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝
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
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <spawn.h>


class RunningApp {
    unsigned long pid = 0;
    long version = 0;
    long containerID = -1;
    std::string appID = "";
    std::string appName = "";
    std::string execPath = "";
    std::string execArgs = "";
    std::string appSource = "";
    std::string updatePath = "";
    std::string role = "";
    std::string updateLastCheck = "";
    std::string updateLastUpdate = "";
    std::string updateLastFail = "";
    std::string updateLastFailReason = "";
    int g_hChildStd_OUT_Rd = 0;
    int g_hChildStd_OUT_Wr = 0;
    int g_hChildStd_ERR_Rd = 0;
    int g_hChildStd_ERR_Wr = 0;
    int g_hChildStd_IN_Rd = 0;
    int g_hChildStd_IN_Wr = 0;
    posix_spawn_file_actions_t g_actions;
public:
    RunningApp(unsigned long pid, const std::string& appID, const std::string& appName, const std::string& execPath, const std::string& execArgs, const std::string& appSource, const std::string& updatePath, const std::string& role, const std::string& updateLastCheck, const std::string& updateLastUpdate, const std::string& updateLastFail, const std::string& updateLastFailReason, long containerID);
    ~RunningApp();
    unsigned long getPID();
    long getVersion();
    long getContainerID();
    void setVersion(long version);
    void setPID(unsigned long pid);
    std::string getAppID();
    std::string getAppName();
    std::string getExecPath();
    std::string getExecName();
    std::string getExecArgs();
    std::string getAppSource();
    std::string getUpdatePath();
    std::string getRole();
    std::string getUpdateLastCheck();
    void setUpdateLastCheck(const std::string& updateLastCheck);
    std::string getUpdateLastUpdate();
    void setUpdateLastUpdate(const std::string& updateLastUpdate);
    std::string getUpdateLastFail();
    void setUpdateLastFail(const std::string& updateLastFail);
    std::string getUpdateLastFailReason();
    void setUpdateLastFailReason(const std::string& updateLastFailReason);
    int getStdOutRead();
    int getStdOutWrite();
    int getStdErrRead();
    int getStdErrWrite();
    int getStdInRead();
    int getStdInWrite();
    posix_spawn_file_actions_t* getActions();
    void setStdOutRead(int fd);
    void setStdOutWrite(int fd);
    void setStdErrRead(int fd);
    void setStdErrWrite(int fd);
    void setStdInRead(int fd);
    void setStdInWrite(int fd);
};