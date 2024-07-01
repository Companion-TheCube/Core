#pragma once
#include <string>

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
public:
    RunningApp(unsigned long pid, std::string appID, std::string appName, std::string execPath, std::string execArgs, std::string appSource, std::string updatePath, std::string role, std::string updateLastCheck, std::string updateLastUpdate, std::string updateLastFail, std::string updateLastFailReason, long containerID);
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
};