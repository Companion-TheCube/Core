#pragma once
#include "dockerApi.h"
#include "nativeApi.h"
#include "./../database/cubeDB.h"
#include "./../utils.h"
#include "RunningApp.h"
#include <poll.h>

class AppsManager {
private:
    std::shared_ptr<DockerAPI> dockerApi;
    std::jthread appsManagerThread;
    TaskQueue taskQueue;
    std::stop_token workerThreadStopToken;
    void appsManagerThreadFn();
    void addWorkerTask(std::function<void()> task);
    std::map<std::string, RunningApp*> runningApps;
    std::vector<std::string> appIDs;
    bool killAbandonedContainers();
    bool killAbandonedProcesses();
    static bool consoleLoggingEnabled;
public:
    AppsManager();
    ~AppsManager();
    bool startApp(std::string appID);
    bool stopApp(std::string appID);
    bool updateApp(std::string appID);
    bool addApp(std::string appID, std::string appName, std::string execPath, std::string execArgs, std::string appSource, std::string updatePath);
    bool removeApp(std::string appID);
    bool updateAppSource(std::string appID, std::string appSource);
    bool updateAppUpdatePath(std::string appID, std::string updatePath);
    bool updateAppExecPath(std::string appID, std::string execPath);
    bool updateAppExecArgs(std::string appID, std::string execArgs);
    bool updateAppUpdateLastCheck(std::string appID, std::string updateLastCheck);
    bool updateAppUpdateLastUpdate(std::string appID, std::string updateLastUpdate);
    bool updateAppUpdateLastFail(std::string appID, std::string updateLastFail);
    bool updateAppUpdateLastFailReason(std::string appID, std::string updateLastFailReason);
    std::string getAppName(std::string appID);
    std::string getAppExecPath(std::string appID);
    std::string getAppExecArgs(std::string appID);
    std::string getAppSource(std::string appID);
    std::string getAppUpdatePath(std::string appID);
    std::string getAppUpdateLastCheck(std::string appID);
    std::string getAppUpdateLastUpdate(std::string appID);
    std::string getAppUpdateLastFail(std::string appID);
    std::string getAppUpdateLastFailReason(std::string appID);
    int getAppCount();
    long getAppVersion(std::string appID);
    long getAppMemoryUsage(std::string appID);
    std::vector<std::string> getAppIDs();
    std::vector<std::string> getAppNames();
    std::string getAppRole(std::string appID);
    bool updateAppRole(std::string appID, std::string role);
    bool addAppRole(std::string appID, std::string role);
    bool removeAppRole(std::string appID);
    bool isAppRunning(std::string appID);
    bool isAppInstalled(std::string appID);
    bool isAppUpdateAvailable(std::string appID);
    bool isAppUpdateFailed(std::string appID);
    bool isAppUpdateCheckOverdue(std::string appID);
    bool isAppUpdateRequired(std::string appID);
    bool startAllApps();
    bool stopAllApps();
    bool updateAllApps();
    void checkAllAppsRunning();
    bool appsManagerThreadRunning();
    static void setConsoleLoggingEnabled(bool enabled);
};