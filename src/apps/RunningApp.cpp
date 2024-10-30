#include "RunningApp.h"

RunningApp::RunningApp(unsigned long pid, const std::string& appID, const std::string& appName, const std::string& execPath, const std::string& execArgs, const std::string& appSource, const std::string& updatePath, const std::string& role, const std::string& updateLastCheck, const std::string& updateLastUpdate, const std::string& updateLastFail, const std::string& updateLastFailReason, long containerID)
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

RunningApp::~RunningApp()
{
#ifdef _WIN32
    if (this->g_hChildStd_OUT_Rd != NULL) {
        CloseHandle(this->g_hChildStd_OUT_Rd);
    }
    if (this->g_hChildStd_OUT_Wr != NULL) {
        CloseHandle(this->g_hChildStd_OUT_Wr);
    }
    if (this->g_hChildStd_ERR_Rd != NULL) {
        CloseHandle(this->g_hChildStd_ERR_Rd);
    }
    if (this->g_hChildStd_ERR_Wr != NULL) {
        CloseHandle(this->g_hChildStd_ERR_Wr);
    }
    if (this->g_hChildStd_IN_Rd != NULL) {
        CloseHandle(this->g_hChildStd_IN_Rd);
    }
    if (this->g_hChildStd_IN_Wr != NULL) {
        CloseHandle(this->g_hChildStd_IN_Wr);
    }
#endif
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
    if (execName.empty()) {
        return "";
    }
    if (execPath.find_last_of("\\") != std::string::npos) {
        if (execName.back() == '\\') {
            execName.pop_back();
        }
        size_t pos = execName.find_last_of("\\");
        if (pos != std::string::npos) {
            execName = execName.substr(pos + 1);
        }
        return execName;
    } else {
        if (execName.back() == '/') {
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

void RunningApp::setUpdateLastCheck(const std::string& updateLastCheck)
{
    this->updateLastCheck = updateLastCheck;
}

void RunningApp::setUpdateLastUpdate(const std::string& updateLastUpdate)
{
    this->updateLastUpdate = updateLastUpdate;
}

void RunningApp::setUpdateLastFail(const std::string& updateLastFail)
{
    this->updateLastFail = updateLastFail;
}

void RunningApp::setUpdateLastFailReason(const std::string& updateLastFailReason)
{
    this->updateLastFailReason = updateLastFailReason;
}

#ifdef _WIN32
HANDLE* RunningApp::getStdOutRead()
{
    return &this->g_hChildStd_OUT_Rd;
}

HANDLE* RunningApp::getStdOutWrite()
{
    return &this->g_hChildStd_OUT_Wr;
}

HANDLE* RunningApp::getStdErrRead()
{
    return &this->g_hChildStd_ERR_Rd;
}

HANDLE* RunningApp::getStdErrWrite()
{
    return &this->g_hChildStd_ERR_Wr;
}

HANDLE* RunningApp::getStdInRead()
{
    return &this->g_hChildStd_IN_Rd;
}

HANDLE* RunningApp::getStdInWrite()
{
    return &this->g_hChildStd_IN_Wr;
}

HANDLE RunningApp::getStdOutReadHandle()
{
    return this->g_hChildStd_OUT_Rd;
}

HANDLE RunningApp::getStdOutWriteHandle()
{
    return this->g_hChildStd_OUT_Wr;
}

HANDLE RunningApp::getStdErrReadHandle()
{
    return this->g_hChildStd_ERR_Rd;
}

HANDLE RunningApp::getStdErrWriteHandle()
{
    return this->g_hChildStd_ERR_Wr;
}

HANDLE RunningApp::getStdInReadHandle()
{
    return this->g_hChildStd_IN_Rd;
}

HANDLE RunningApp::getStdInWriteHandle()
{
    return this->g_hChildStd_IN_Wr;
}
#endif

#ifdef __linux__
int RunningApp::getStdOutRead()
{
    return this->g_hChildStd_OUT_Rd;
}

int RunningApp::getStdOutWrite()
{
    return this->g_hChildStd_OUT_Wr;
}

int RunningApp::getStdErrRead()
{
    return this->g_hChildStd_ERR_Rd;
}

int RunningApp::getStdErrWrite()
{
    return this->g_hChildStd_ERR_Wr;
}

int RunningApp::getStdInRead()
{
    return this->g_hChildStd_IN_Rd;
}

int RunningApp::getStdInWrite()
{
    return this->g_hChildStd_IN_Wr;
}

posix_spawn_file_actions_t* RunningApp::getActions()
{
    return &this->g_actions;
}

void RunningApp::setStdOutRead(int fd)
{
    this->g_hChildStd_OUT_Rd = fd;
}

void RunningApp::setStdOutWrite(int fd)
{
    this->g_hChildStd_OUT_Wr = fd;
}

void RunningApp::setStdErrRead(int fd)
{
    this->g_hChildStd_ERR_Rd = fd;
}

void RunningApp::setStdErrWrite(int fd)
{
    this->g_hChildStd_ERR_Wr = fd;
}

void RunningApp::setStdInRead(int fd)
{
    this->g_hChildStd_IN_Rd = fd;
}

void RunningApp::setStdInWrite(int fd)
{
    this->g_hChildStd_IN_Wr = fd;
}
#endif