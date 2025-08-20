/*
 █████╗ ██████╗ ██████╗ ███████╗███╗   ███╗ █████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗    ██╗  ██╗
██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗   ██║  ██║
███████║██████╔╝██████╔╝███████╗██╔████╔██║███████║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝   ███████║
██╔══██║██╔═══╝ ██╔═══╝ ╚════██║██║╚██╔╝██║██╔══██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗   ██╔══██║
██║  ██║██║     ██║     ███████║██║ ╚═╝ ██║██║  ██║██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║██╗██║  ██║
╚═╝  ╚═╝╚═╝     ╚═╝     ╚══════╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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
#include "./../database/cubeDB.h"
#include "./../utils.h"
#include "RunningApp.h"
#include "dockerApi.h"
#include "nativeApi.h"
#include <poll.h>

class AppsManager {
private:
    std::shared_ptr<DockerAPI> dockerApi;
    std::jthread appsManagerThread;
    TaskQueue<std::function<void()>> taskQueue;
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
    static const std::vector<std::string> getAppNames_static(){
        CubeLog::info("Getting app names.");
        std::vector<std::vector<std::string>> data = CubeDB::getDBManager()->getDatabase("apps")->selectData("apps", { "app_name" });
        std::vector<std::string> appNames;
        for (size_t i = 0; i < data.size(); i++) {
            appNames.push_back(data[i][0]);
        }
        return appNames;
    }
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

class Installer {
public:
    static bool installApp(const std::string& appID);
    static bool uninstallApp(const std::string& appID);
    static bool updateApp(const std::string& appID);
    static bool isAppInstalled(const std::string& appID);
};