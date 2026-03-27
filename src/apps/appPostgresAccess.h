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
    constexpr std::string getInterfaceName() const override
    {
        return "AppPostgresAccess";
    }
};
