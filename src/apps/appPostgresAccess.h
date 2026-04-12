/*
 █████╗ ██████╗ ██████╗ ██████╗  ██████╗ ███████╗████████╗ ██████╗ ██████╗ ███████╗███████╗ █████╗  ██████╗ ██████╗███████╗███████╗███████╗   ██╗  ██╗
██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔═══██╗██╔════╝╚══██╔══╝██╔════╝ ██╔══██╗██╔════╝██╔════╝██╔══██╗██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝   ██║  ██║
███████║██████╔╝██████╔╝██████╔╝██║   ██║███████╗   ██║   ██║  ███╗██████╔╝█████╗  ███████╗███████║██║     ██║     █████╗  ███████╗███████╗   ███████║
██╔══██║██╔═══╝ ██╔═══╝ ██╔═══╝ ██║   ██║╚════██║   ██║   ██║   ██║██╔══██╗██╔══╝  ╚════██║██╔══██║██║     ██║     ██╔══╝  ╚════██║╚════██║   ██╔══██║
██║  ██║██║     ██║     ██║     ╚██████╔╝███████║   ██║   ╚██████╔╝██║  ██║███████╗███████║██║  ██║╚██████╗╚██████╗███████╗███████║███████║██╗██║  ██║
╚═╝  ╚═╝╚═╝     ╚═╝     ╚═╝      ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝╚══════╝╚══════╝╚══════╝╚═╝╚═╝  ╚═╝
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

#include "../api/api.h"
#include "../database/cubeDB.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class AppPostgresAdminClient {
public:
    virtual ~AppPostgresAdminClient() = default;
    virtual bool ensureProvisioned(
        const std::string& roleName,
        const std::string& password,
        const std::vector<std::string>& desiredDatabases,
        const std::vector<std::string>& removedDatabases,
        std::string& error) = 0;
};

class AppPostgresAccess : public AutoRegisterAPI<AppPostgresAccess> {
public:
    AppPostgresAccess() = default;
    ~AppPostgresAccess() override = default;

    static bool parseRequestedDatabases(const nlohmann::json& manifest, std::vector<std::string>& logicalDatabases, std::string& error);
    static bool provisionForApp(
        const std::string& appId,
        const nlohmann::json& manifest,
        const std::function<bool(const std::string&, std::string&)>& ensureSystemAppRunning,
        std::string& error);
    static bool appendLaunchEnvironment(
        const std::string& appId,
        const std::vector<std::string>& logicalDatabases,
        nlohmann::json& environmentSet,
        std::string& error);

    static void setAdminClientForTests(std::shared_ptr<AppPostgresAdminClient> adminClient);
    static void clearAdminClientForTests();

    static std::string bootstrapHeaderName();
    static std::string bootstrapEnvVarName();
    static std::string endpointEnvVarName();

    HttpEndPointData_t getHttpEndpointData() override;
    std::string getInterfaceName() const override
    {
        return "AppPostgresAccess";
    }
};
