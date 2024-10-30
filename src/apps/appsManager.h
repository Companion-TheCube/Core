#pragma once
#include "./../database/cubeDB.h"
#include "./../utils.h"
#include "RunningApp.h"
#include "dockerApi.h"
#include "nativeApi.h"
#ifdef __linux__
#include <poll.h>
#endif

class AppsManager {
private:
    std::shared_ptr<DockerAPI> dockerApi;
    std::jthread appsManagerThread;
    TaskQueue taskQueue;
    std::stop_token workerThreadStopToken;
    void appsManagerThreadFn();
    void addWorkerTask(std::function<void()> task);
    std::map<std::string, std::shared_ptr<RunningApp>> runningApps;
    std::vector<std::string> appIDs = {};
    bool killAbandonedContainers();
    bool killAbandonedProcesses();
    static bool consoleLoggingEnabled;

public:
    AppsManager();
    ~AppsManager();
    bool startApp(const std::string& appID);
    bool stopApp(const std::string& appID);
    bool updateApp(const std::string& appID);
    bool addApp(const std::string& appID, const std::string& appName, const std::string& execPath, const std::string& execArgs, const std::string& appSource, const std::string& updatePath);
    bool removeApp(const std::string& appID);
    bool updateAppSource(const std::string& appID, const std::string& appSource);
    bool updateAppUpdatePath(const std::string& appID, const std::string& updatePath);
    bool updateAppExecPath(const std::string& appID, const std::string& execPath);
    bool updateAppExecArgs(const std::string& appID, const std::string& execArgs);
    bool updateAppUpdateLastCheck(const std::string& appID, const std::string& updateLastCheck);
    bool updateAppUpdateLastUpdate(const std::string& appID, const std::string& updateLastUpdate);
    bool updateAppUpdateLastFail(const std::string& appID, const std::string& updateLastFail);
    bool updateAppUpdateLastFailReason(const std::string& appID, const std::string& updateLastFailReason);
    std::string getAppName(const std::string& appID);
    std::string getAppExecPath(const std::string& appID);
    std::string getAppExecArgs(const std::string& appID);
    std::string getAppSource(const std::string& appID);
    std::string getAppUpdatePath(const std::string& appID);
    std::string getAppUpdateLastCheck(const std::string& appID);
    std::string getAppUpdateLastUpdate(const std::string& appID);
    std::string getAppUpdateLastFail(const std::string& appID);
    std::string getAppUpdateLastFailReason(const std::string& appID);
    int getAppCount();
    long getAppVersion(const std::string& appID);
    long getAppMemoryUsage(const std::string& appID);
    std::vector<std::string> getAppIDs();
    std::vector<std::string> getAppNames();
    std::string getAppRole(const std::string& appID);
    bool updateAppRole(const std::string& appID, const std::string& role);
    bool addAppRole(const std::string& appID, const std::string& role);
    bool removeAppRole(const std::string& appID);
    bool isAppRunning(const std::string& appID);
    bool isAppInstalled(const std::string& appID);
    bool isAppUpdateAvailable(const std::string& appID);
    bool isAppUpdateFailed(const std::string& appID);
    bool isAppUpdateCheckOverdue(const std::string& appID);
    bool isAppUpdateRequired(const std::string& appID);
    bool startAllApps();
    bool stopAllApps();
    bool updateAllApps();
    void checkAllAppsRunning();
    bool appsManagerThreadRunning();
    static void setConsoleLoggingEnabled(bool enabled);
};