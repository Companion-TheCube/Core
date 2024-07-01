#include "RunningApp.h"

RunningApp::RunningApp(unsigned long pid, std::string appID, std::string appName, std::string execPath, std::string execArgs, std::string appSource, std::string updatePath, std::string role, std::string updateLastCheck, std::string updateLastUpdate, std::string updateLastFail, std::string updateLastFailReason, long containerID)
{
    this->pid = pid;
    this->appID = appID;
    this->appName = appName;
    this->execPath = execPath;
    this->execArgs = execArgs;
    this->appSource = appSource;
    this->updatePath = updatePath;
    this->role = role;
    this->updateLastCheck = updateLastCheck;
    this->updateLastUpdate = updateLastUpdate;
    this->updateLastFail = updateLastFail;
    this->updateLastFailReason = updateLastFailReason;
    this->containerID = containerID;
}

unsigned long RunningApp::getPID()
{
    return this->pid;
}

std::string RunningApp::getAppID()
{
    return this->appID;
}

std::string RunningApp::getAppName()
{
    return this->appName;
}

std::string RunningApp::getExecPath()
{
    return this->execPath;
}

std::string RunningApp::getExecName()
{
    std::string execName = this->execPath;
    if(execName.empty()) {
        return "";
    }
    if(execPath.find_last_of("\\") != std::string::npos) {
        if(execName.back() == '\\') {
            execName.pop_back();
        }
        size_t pos = execName.find_last_of("\\");
        if (pos != std::string::npos) {
            execName = execName.substr(pos + 1);
        }
        return execName;
    } else {
        if(execName.back() == '/') {
            execName.pop_back();
        }
        size_t pos = execName.find_last_of("/");
        if (pos != std::string::npos) {
            execName = execName.substr(pos + 1);
        }
        return execName;
    }
}

std::string RunningApp::getExecArgs()
{
    return this->execArgs;
}

std::string RunningApp::getAppSource()
{
    return this->appSource;
}

std::string RunningApp::getUpdatePath()
{
    return this->updatePath;
}

std::string RunningApp::getRole()
{
    return this->role;
}

std::string RunningApp::getUpdateLastCheck()
{
    return this->updateLastCheck;
}

std::string RunningApp::getUpdateLastUpdate()
{
    return this->updateLastUpdate;
}

std::string RunningApp::getUpdateLastFail()
{
    return this->updateLastFail;
}

std::string RunningApp::getUpdateLastFailReason()
{
    return this->updateLastFailReason;
}

long RunningApp::getContainerID()
{
    return this->containerID;
}

void RunningApp::setPID(unsigned long pid)
{
    this->pid = pid;
}

void RunningApp::setVersion(long version)
{
    this->version = version;
}

void RunningApp::setUpdateLastCheck(std::string updateLastCheck)
{
    this->updateLastCheck = updateLastCheck;
}

void RunningApp::setUpdateLastUpdate(std::string updateLastUpdate)
{
    this->updateLastUpdate = updateLastUpdate;
}

void RunningApp::setUpdateLastFail(std::string updateLastFail)
{
    this->updateLastFail = updateLastFail;
}

void RunningApp::setUpdateLastFailReason(std::string updateLastFailReason)
{
    this->updateLastFailReason = updateLastFailReason;
}