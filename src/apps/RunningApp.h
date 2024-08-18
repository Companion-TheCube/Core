#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#ifdef __linux__
#include <spawn.h>
#endif
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif

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
#ifdef _WIN32
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
    HANDLE g_hChildStd_ERR_Rd = NULL;
    HANDLE g_hChildStd_ERR_Wr = NULL;
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
#endif
#ifdef __linux__
    int g_hChildStd_OUT_Rd = 0;
    int g_hChildStd_OUT_Wr = 0;
    int g_hChildStd_ERR_Rd = 0;
    int g_hChildStd_ERR_Wr = 0;
    int g_hChildStd_IN_Rd = 0;
    int g_hChildStd_IN_Wr = 0;
    posix_spawn_file_actions_t g_actions;
#endif
public:
    RunningApp(unsigned long pid, std::string appID, std::string appName, std::string execPath, std::string execArgs, std::string appSource, std::string updatePath, std::string role, std::string updateLastCheck, std::string updateLastUpdate, std::string updateLastFail, std::string updateLastFailReason, long containerID);
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
    void setUpdateLastCheck(std::string updateLastCheck);
    std::string getUpdateLastUpdate();
    void setUpdateLastUpdate(std::string updateLastUpdate);
    std::string getUpdateLastFail();
    void setUpdateLastFail(std::string updateLastFail);
    std::string getUpdateLastFailReason();
    void setUpdateLastFailReason(std::string updateLastFailReason);
#ifdef _WIN32
    HANDLE* getStdOutRead();
    HANDLE* getStdOutWrite();
    HANDLE* getStdErrRead();
    HANDLE* getStdErrWrite();
    HANDLE* getStdInRead();
    HANDLE* getStdInWrite();
    HANDLE getStdOutReadHandle();
    HANDLE getStdOutWriteHandle();
    HANDLE getStdErrReadHandle();
    HANDLE getStdErrWriteHandle();
    HANDLE getStdInReadHandle();
    HANDLE getStdInWriteHandle();
#endif
#ifdef __linux__
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
#endif
};