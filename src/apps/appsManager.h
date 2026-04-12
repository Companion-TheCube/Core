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

Copyright (c) 2026 A-McD Technology LLC

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
#include <memory>
#include <string>
#include <vector>

class AppRuntimeController {
public:
    virtual ~AppRuntimeController() = default;
    virtual bool startUnit(const std::string& unitName, std::string* errorOut = nullptr) = 0;
    virtual bool stopUnit(const std::string& unitName, std::string* errorOut = nullptr) = 0;
    virtual bool isUnitActive(const std::string& unitName, std::string* errorOut = nullptr) const = 0;
};

class SystemdAppRuntimeController : public AppRuntimeController {
public:
    bool startUnit(const std::string& unitName, std::string* errorOut = nullptr) override;
    bool stopUnit(const std::string& unitName, std::string* errorOut = nullptr) override;
    bool isUnitActive(const std::string& unitName, std::string* errorOut = nullptr) const override;
};

class AppsManager {
public:
    AppsManager();
    explicit AppsManager(std::shared_ptr<AppRuntimeController> runtimeController);
    ~AppsManager();

    bool initialize();
    void shutdown();
    bool startApp(const std::string& appID);
    bool stopApp(const std::string& appID);
    bool isAppRunning(const std::string& appID) const;
    bool isAppInstalled(const std::string& appID) const;

    std::vector<std::string> getAppIDs() const;
    std::vector<std::string> getAppNames() const;

    static std::vector<std::string> getAppNames_static();

private:
    std::shared_ptr<AppRuntimeController> runtimeController;
    bool initialized = false;
    bool shutdownComplete = false;

    bool waitForDatabaseManager() const;
    bool syncRegistry();
    bool launchStartupApps();
};
