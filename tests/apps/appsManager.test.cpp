#include <gtest/gtest.h>

#include "../src/apps/appPostgresAccess.h"
#include "../src/apps/appsManager.h"
#include "../src/database/cubeDB.h"

#include <chrono>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace {
class FakeRuntimeController : public AppRuntimeController {
public:
    bool startShouldSucceed = true;
    bool stopShouldSucceed = true;
    std::set<std::string> activeUnits;
    std::vector<std::string> startedUnits;
    std::vector<std::string> stoppedUnits;

    bool startUnit(const std::string& unitName, std::string* errorOut = nullptr) override
    {
        startedUnits.push_back(unitName);
        if (!startShouldSucceed) {
            if (errorOut != nullptr) {
                *errorOut = "fake start failure";
            }
            return false;
        }
        activeUnits.insert(unitName);
        return true;
    }

    bool stopUnit(const std::string& unitName, std::string* errorOut = nullptr) override
    {
        stoppedUnits.push_back(unitName);
        if (!stopShouldSucceed) {
            if (errorOut != nullptr) {
                *errorOut = "fake stop failure";
            }
            return false;
        }
        activeUnits.erase(unitName);
        return true;
    }

    bool isUnitActive(const std::string& unitName, std::string* errorOut = nullptr) const override
    {
        if (activeUnits.count(unitName) > 0) {
            return true;
        }
        if (errorOut != nullptr) {
            *errorOut = "inactive";
        }
        return false;
    }
};

class FakePostgresAdminClient : public AppPostgresAdminClient {
public:
    struct Call {
        std::string roleName;
        std::string password;
        std::vector<std::string> desiredDatabases;
        std::vector<std::string> removedDatabases;
    };

    bool shouldSucceed = true;
    std::vector<Call> calls;

    bool ensureProvisioned(
        const std::string& roleName,
        const std::string& password,
        const std::vector<std::string>& desiredDatabases,
        const std::vector<std::string>& removedDatabases,
        std::string& error) override
    {
        calls.push_back({ roleName, password, desiredDatabases, removedDatabases });
        if (!shouldSucceed) {
            error = "fake postgres provisioning failure";
            return false;
        }
        return true;
    }
};

class AppsManagerTest : public ::testing::Test {
protected:
    fs::path originalCwd;
    fs::path tempRoot;
    fs::path installRootsDir;
    fs::path launchRoot;
    fs::path dataRoot;
    fs::path cacheRoot;
    fs::path runtimeRoot;
    std::shared_ptr<CubeDatabaseManager> dbManager;
    std::shared_ptr<BlobsManager> blobsManager;
    std::shared_ptr<CubeDB> cubeDb;

    void SetUp() override
    {
        originalCwd = fs::current_path();
        tempRoot = fs::temp_directory_path() / ("apps_manager_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        installRootsDir = tempRoot / "install-roots";
        launchRoot = tempRoot / "launch";
        dataRoot = tempRoot / "managed-data";
        cacheRoot = tempRoot / "managed-cache";
        runtimeRoot = tempRoot / "managed-runtime";

        fs::create_directories(tempRoot / "data");
        fs::create_directories(installRootsDir);
        fs::create_directories(dataRoot);
        fs::create_directories(cacheRoot);
        fs::create_directories(runtimeRoot);
        fs::current_path(tempRoot);

        Config::set("APP_INSTALL_ROOTS", installRootsDir.string());
        Config::set("THECUBE_LAUNCH_ROOT", launchRoot.string());
        Config::set("THECUBE_DATA_ROOT", dataRoot.string());
        Config::set("THECUBE_CACHE_ROOT", cacheRoot.string());
        Config::set("THECUBE_RUNTIME_ROOT", runtimeRoot.string());
        Config::set("THECUBE_DOCKER_BIN", "");
        Config::set("THECUBE_HOST_PYTHON_BIN", "");
    }

    void TearDown() override
    {
        AppPostgresAccess::clearAdminClientForTests();
        CubeDB::setCubeDBManager(nullptr);
        CubeDB::setBlobsManager(nullptr);
        cubeDb.reset();
        blobsManager.reset();
        dbManager.reset();

        fs::current_path(originalCwd);
        fs::remove_all(tempRoot);
    }

    void initializeDatabaseContext()
    {
        dbManager = std::make_shared<CubeDatabaseManager>();
        blobsManager = std::make_shared<BlobsManager>(dbManager, "data/blobs.db");
        cubeDb = std::make_shared<CubeDB>(dbManager, blobsManager);
    }

    fs::path createPythonApp(
        const std::string& appId,
        bool systemApp,
        bool autostart,
        bool validManifest = true,
        bool createVenv = true,
        const std::string& packageName = "",
        const std::vector<std::string>& postgresDatabases = {})
    {
        const fs::path appRoot = installRootsDir / appId;
        fs::create_directories(appRoot);

        if (createVenv) {
            fs::create_directories(appRoot / ".venv/bin");
            std::ofstream(appRoot / ".venv/bin/python") << "#!/usr/bin/env python3\n";
        }
        std::ofstream(appRoot / "main.py") << "print('hello')\n";

        nlohmann::json pythonRuntime = {
            { "entry_script", "main.py" }
        };
        if (!packageName.empty()) {
            pythonRuntime["package_name"] = packageName;
        }

        nlohmann::json manifest = {
            { "schema_version", "1.0" },
            { "app", {
                  { "id", appId },
                  { "name", appId + " name" },
                  { "version", "0.1.0" },
                  { "type", "service" },
                  { "working_directory", "." },
                  { "autostart", autostart },
                  { "system_app", systemApp }
              } },
            { "runtime", {
                  { "type", "python" },
                  { "distribution", "venv" },
                  { "compatibility", "3.11" },
                  { "python", pythonRuntime }
              } },
            { "permissions", {
                  { "filesystem", {
                        { "read_only", nlohmann::json::array({ "app://install" }) },
                        { "read_write", nlohmann::json::array({ "app://runtime", "app://cache" }) }
                    } },
                  { "platform", nlohmann::json::array() },
                  { "network", {
                        { "allow", false }
                    } }
              } },
            { "environment", {
                  { "allow_inherit", nlohmann::json::array() },
                  { "set", {
                        { "TEST_FLAG", "1" }
                    } }
              } }
        };

        if (!postgresDatabases.empty()) {
            manifest["permissions"]["postgresql"] = nlohmann::json {
                { "databases", postgresDatabases }
            };
        }

        if (!validManifest) {
            manifest.erase("permissions");
        }

        std::ofstream(appRoot / "manifest.json") << manifest.dump(2);
        return appRoot;
    }

    fs::path createDockerApp(const std::string& appId, bool systemApp)
    {
        const fs::path appRoot = installRootsDir / appId;
        fs::create_directories(appRoot);

        const fs::path fakeDocker = tempRoot / "bin/docker";
        fs::create_directories(fakeDocker.parent_path());
        std::ofstream(fakeDocker) << "#!/bin/sh\nexit 0\n";
        Config::set("THECUBE_DOCKER_BIN", fakeDocker.string());

        nlohmann::json manifest = {
            { "schema_version", "1.0" },
            { "app", {
                  { "id", appId },
                  { "name", "PostgreSQL" },
                  { "version", "16.0" },
                  { "type", "background" },
                  { "working_directory", "." },
                  { "autostart", false },
                  { "system_app", systemApp },
                  { "args", nlohmann::json::array({ "-c", "listen_addresses=*" }) }
              } },
            { "runtime", {
                  { "type", "docker" },
                  { "distribution", "container-image" },
                  { "compatibility", "16" },
                  { "docker", {
                        { "image", "postgres:16" },
                        { "container_name", "thecube-postgresql" },
                        { "published_ports", nlohmann::json::array({ "127.0.0.1:5432:5432" }) },
                        { "environment", {
                              { "POSTGRES_USER", "postgres" },
                              { "POSTGRES_PASSWORD", "thecube-postgres-admin" },
                              { "POSTGRES_DB", "postgres" }
                          } },
                        { "volumes", nlohmann::json::array({
                              nlohmann::json {
                                  { "host", "app://data" },
                                  { "container", "/var/lib/postgresql/data" }
                              }
                          }) }
                    } }
              } },
            { "permissions", {
                  { "filesystem", {
                        { "read_only", nlohmann::json::array({ "app://install" }) },
                        { "read_write", nlohmann::json::array({ "app://data", "app://runtime" }) }
                    } },
                  { "platform", nlohmann::json::array() },
                  { "network", {
                        { "allow", false }
                    } }
              } }
        };

        std::ofstream(appRoot / "manifest.json") << manifest.dump(2);
        return appRoot;
    }

    fs::path createFakeHostPython()
    {
        const fs::path fakePython = tempRoot / "bin/fake-python3";
        fs::create_directories(fakePython.parent_path());

        std::ofstream script(fakePython);
        script
            << "#!/bin/sh\n"
            << "set -e\n"
            << "if [ \"$1\" = \"-m\" ] && [ \"$2\" = \"venv\" ]; then\n"
            << "  target=\"$3\"\n"
            << "  mkdir -p \"$target/bin\"\n"
            << "  cat > \"$target/bin/python3\" <<'EOF_PY'\n"
            << "#!/bin/sh\n"
            << "exit 0\n"
            << "EOF_PY\n"
            << "  chmod +x \"$target/bin/python3\"\n"
            << "  cat > \"$target/bin/pip\" <<'EOF_PIP'\n"
            << "#!/bin/sh\n"
            << "echo \"$@\" >> \"$(dirname \"$0\")/pip.log\"\n"
            << "exit 0\n"
            << "EOF_PIP\n"
            << "  chmod +x \"$target/bin/pip\"\n"
            << "  exit 0\n"
            << "fi\n"
            << "exit 1\n";
        script.close();

        fs::permissions(
            fakePython,
            fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write
                | fs::perms::group_exec | fs::perms::group_read
                | fs::perms::others_exec | fs::perms::others_read,
            fs::perm_options::replace);

        Config::set("THECUBE_HOST_PYTHON_BIN", fakePython.string());
        return fakePython;
    }

    std::vector<std::vector<std::string>> selectAppRow(const std::string& appId, const std::vector<std::string>& columns)
    {
        return dbManager->getDatabase("apps")->selectData("apps", columns, { DB_NS::Predicate { "app_id", appId } });
    }
};
} // namespace

TEST_F(AppsManagerTest, InitializeSyncsRegistryAndStartsSystemApps)
{
    createPythonApp("com.example.systemapp", true, false);
    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    ASSERT_EQ(runtime->startedUnits.size(), 1u);
    EXPECT_EQ(runtime->startedUnits[0], "thecube-app@com.example.systemapp.service");
    EXPECT_TRUE(fs::exists(launchRoot / "com.example.systemapp/launch-policy.json"));

    const auto rows = selectAppRow(
        "com.example.systemapp",
        { "app_name", "manifest_path", "is_system_app", "autostart", "policy_compile_status", "last_started_at" });
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0], "com.example.systemapp name");
    EXPECT_EQ(rows[0][2], "1");
    EXPECT_EQ(rows[0][3], "0");
    EXPECT_EQ(rows[0][4], "compiled");
    EXPECT_FALSE(rows[0][5].empty());
}

TEST_F(AppsManagerTest, InitializeUsesExecutableDirectoryAppsWhenInstallRootsUnset)
{
    Config::set("APP_INSTALL_ROOTS", "");

    std::error_code ec;
    const fs::path executableDir = fs::read_symlink("/proc/self/exe", ec).parent_path();
    ASSERT_FALSE(ec);

    const fs::path appRoot = executableDir / "apps/com.example.cwd";
    fs::create_directories(appRoot / ".venv/bin");
    std::ofstream(appRoot / ".venv/bin/python") << "#!/usr/bin/env python3\n";
    std::ofstream(appRoot / "main.py") << "print('cwd app')\n";
    std::ofstream(appRoot / "manifest.json") << nlohmann::json({
        { "schema_version", "1.0" },
        { "app", {
              { "id", "com.example.cwd" },
              { "name", "cwd app" },
              { "version", "0.1.0" },
              { "type", "background" },
              { "working_directory", "." },
              { "autostart", false },
              { "system_app", true }
          } },
        { "runtime", {
              { "type", "python" },
              { "distribution", "venv" },
              { "compatibility", "3.11" },
              { "python", {
                    { "entry_script", "main.py" }
                } }
          } },
        { "permissions", {
              { "filesystem", {
                    { "read_only", nlohmann::json::array({ "app://install" }) },
                    { "read_write", nlohmann::json::array({ "app://runtime" }) }
                } }
          } }
    }).dump(2);

    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    EXPECT_FALSE(runtime->startedUnits.empty());
    EXPECT_NE(
        std::find(runtime->startedUnits.begin(), runtime->startedUnits.end(), "thecube-app@com.example.cwd.service"),
        runtime->startedUnits.end());
    EXPECT_TRUE(fs::exists(launchRoot / "com.example.cwd/launch-policy.json"));

    fs::remove_all(appRoot);
    if (fs::exists(appRoot.parent_path()) && fs::is_empty(appRoot.parent_path())) {
        fs::remove(appRoot.parent_path());
    }
}

TEST_F(AppsManagerTest, InitializeRecordsInvalidManifestAndSkipsStartup)
{
    createPythonApp("com.example.invalid", true, false, false);
    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_FALSE(manager.initialize());
    EXPECT_TRUE(runtime->startedUnits.empty());

    const auto rows = selectAppRow(
        "com.example.invalid",
        { "policy_compile_status", "policy_compile_error" });
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0], "error");
    EXPECT_NE(rows[0][1].find("Missing permissions object"), std::string::npos);
}

TEST_F(AppsManagerTest, InitializeRemovesRegistryRowsForDeletedManifests)
{
    const auto appRoot = createPythonApp("com.example.cleanup", false, false);
    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    EXPECT_EQ(selectAppRow("com.example.cleanup", { "app_id" }).size(), 1u);

    fs::remove(appRoot / "manifest.json");
    EXPECT_TRUE(manager.initialize());
    EXPECT_TRUE(selectAppRow("com.example.cleanup", { "app_id" }).empty());
}

TEST_F(AppsManagerTest, StartStopAndRunningDelegateToRuntimeController)
{
    createPythonApp("com.example.manual", false, false);
    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    EXPECT_TRUE(runtime->startedUnits.empty());

    EXPECT_TRUE(manager.startApp("com.example.manual"));
    EXPECT_TRUE(manager.isAppRunning("com.example.manual"));
    EXPECT_TRUE(manager.stopApp("com.example.manual"));
    EXPECT_FALSE(manager.isAppRunning("com.example.manual"));

    ASSERT_EQ(runtime->startedUnits.size(), 1u);
    ASSERT_EQ(runtime->stoppedUnits.size(), 1u);
    EXPECT_EQ(runtime->startedUnits[0], "thecube-app@com.example.manual.service");
    EXPECT_EQ(runtime->stoppedUnits[0], "thecube-app@com.example.manual.service");
}

TEST_F(AppsManagerTest, StartAppBootstrapsManagedPythonVenvWhenMissing)
{
    createPythonApp("com.example.bootstrap", false, false, true, false, "example-bootstrap");
    createFakeHostPython();
    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    EXPECT_TRUE(manager.startApp("com.example.bootstrap"));

    const fs::path venvRoot = dataRoot / "com.example.bootstrap/venv";
    const fs::path pipLog = venvRoot / "bin/pip.log";
    const fs::path statePath = dataRoot / "com.example.bootstrap/python-install-state.json";
    const fs::path policyPath = launchRoot / "com.example.bootstrap/launch-policy.json";

    EXPECT_TRUE(fs::exists(venvRoot / "bin/python3"));
    EXPECT_TRUE(fs::exists(pipLog));
    EXPECT_TRUE(fs::exists(statePath));
    EXPECT_TRUE(fs::exists(policyPath));

    const std::string pipLogText = [] (const fs::path& path) {
        std::ifstream input(path);
        std::stringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }(pipLog);
    EXPECT_NE(pipLogText.find("install --upgrade pip setuptools wheel"), std::string::npos);
    EXPECT_NE(pipLogText.find("install example-bootstrap"), std::string::npos);

    std::ifstream input(policyPath);
    nlohmann::json policy;
    input >> policy;
    EXPECT_EQ(policy["app"]["argv"][0], (venvRoot / "bin/python3").string());
}

TEST_F(AppsManagerTest, StartAppSkipsPythonBootstrapWhenInstallStateMatches)
{
    createPythonApp("com.example.reuse", false, false, true, false, "example-reuse");
    createFakeHostPython();
    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    EXPECT_TRUE(manager.startApp("com.example.reuse"));

    const fs::path pipLog = dataRoot / "com.example.reuse/venv/bin/pip.log";
    ASSERT_TRUE(fs::exists(pipLog));

    const auto readText = [](const fs::path& path) {
        std::ifstream input(path);
        std::stringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    };

    const auto firstLog = readText(pipLog);
    EXPECT_TRUE(manager.startApp("com.example.reuse"));
    const auto secondLog = readText(pipLog);

    EXPECT_EQ(firstLog, secondLog);
}

TEST_F(AppsManagerTest, InitializeCompilesDockerSystemAppPolicy)
{
    createDockerApp("com.thecube.postgresql", true);
    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    ASSERT_EQ(runtime->startedUnits.size(), 1u);
    EXPECT_EQ(runtime->startedUnits[0], "thecube-app@com.thecube.postgresql.service");

    const fs::path policyPath = launchRoot / "com.thecube.postgresql/launch-policy.json";
    ASSERT_TRUE(fs::exists(policyPath));

    std::ifstream input(policyPath);
    nlohmann::json policy;
    input >> policy;

    EXPECT_EQ(policy["runtime"]["type"], "docker");
    EXPECT_EQ(policy["runtime"]["resolved_executable"], (tempRoot / "bin/docker").string());
    EXPECT_EQ(policy["runtime"]["launch_target"], "postgres:16");
    EXPECT_EQ(policy["app"]["argv"][0], (tempRoot / "bin/docker").string());
    EXPECT_EQ(policy["app"]["argv"][1], "run");
    bool sawImage = false;
    bool sawAdminPassword = false;
    bool sawTrustAuth = false;
    for (const auto& arg : policy["app"]["argv"]) {
        if (!arg.is_string()) {
            continue;
        }
        const auto value = arg.get<std::string>();
        if (value == "postgres:16") {
            sawImage = true;
        }
        if (value == "POSTGRES_PASSWORD=thecube-postgres-admin") {
            sawAdminPassword = true;
        }
        if (value == "POSTGRES_HOST_AUTH_METHOD=trust") {
            sawTrustAuth = true;
        }
    }
    EXPECT_TRUE(sawImage);
    EXPECT_TRUE(sawAdminPassword);
    EXPECT_FALSE(sawTrustAuth);
}

TEST_F(AppsManagerTest, InitializeProvisionedPostgresAccessAndInjectsBootstrapToken)
{
    createDockerApp("com.thecube.postgresql", true);
    createPythonApp("com.example.dataapp", false, false, true, true, "", { "main", "analytics" });
    initializeDatabaseContext();

    auto fakeAdmin = std::make_shared<FakePostgresAdminClient>();
    AppPostgresAccess::setAdminClientForTests(fakeAdmin);

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_TRUE(manager.initialize());
    EXPECT_TRUE(manager.startApp("com.example.dataapp"));

    ASSERT_GE(fakeAdmin->calls.size(), 1u);
    const auto& lastCall = fakeAdmin->calls.back();
    EXPECT_FALSE(lastCall.roleName.empty());
    EXPECT_EQ(lastCall.desiredDatabases.size(), 2u);
    EXPECT_TRUE(lastCall.removedDatabases.empty());

    const auto accessRows = dbManager->getDatabase("apps")->selectData(
        "app_postgresql_access",
        { "app_id", "pg_role_name", "database_mapping_json", "bootstrap_token_hash" },
        { DB_NS::Predicate { "app_id", "com.example.dataapp" } });
    ASSERT_EQ(accessRows.size(), 1u);
    EXPECT_EQ(accessRows[0][0], "com.example.dataapp");
    EXPECT_EQ(accessRows[0][1], lastCall.roleName);
    EXPECT_FALSE(accessRows[0][2].empty());
    EXPECT_FALSE(accessRows[0][3].empty());

    const fs::path policyPath = launchRoot / "com.example.dataapp/launch-policy.json";
    ASSERT_TRUE(fs::exists(policyPath));
    std::ifstream input(policyPath);
    nlohmann::json policy;
    input >> policy;

    const auto& envSet = policy["environment"]["set"];
    EXPECT_TRUE(envSet.contains("THECUBE_POSTGRES_BOOTSTRAP_TOKEN"));
    EXPECT_TRUE(envSet.contains("THECUBE_POSTGRES_CREDENTIALS_ENDPOINT"));
    EXPECT_EQ(envSet["THECUBE_POSTGRES_CREDENTIALS_ENDPOINT"], "/AppPostgresAccess-fetchCredentials");
}

TEST_F(AppsManagerTest, InitializeRejectsInvalidPostgresDatabasePermissionNames)
{
    const fs::path appRoot = installRootsDir / "com.example.invalidpg";
    fs::create_directories(appRoot / ".venv/bin");
    std::ofstream(appRoot / ".venv/bin/python") << "#!/usr/bin/env python3\n";
    std::ofstream(appRoot / "main.py") << "print('hello')\n";
    std::ofstream(appRoot / "manifest.json") << nlohmann::json({
        { "schema_version", "1.0" },
        { "app", {
              { "id", "com.example.invalidpg" },
              { "name", "invalid pg app" },
              { "version", "0.1.0" },
              { "type", "service" },
              { "working_directory", "." },
              { "autostart", false },
              { "system_app", false }
          } },
        { "runtime", {
              { "type", "python" },
              { "distribution", "venv" },
              { "compatibility", "3.11" },
              { "python", {
                    { "entry_script", "main.py" }
                } }
          } },
        { "permissions", {
              { "filesystem", {
                    { "read_only", nlohmann::json::array({ "app://install" }) },
                    { "read_write", nlohmann::json::array({ "app://runtime" }) }
                } },
              { "postgresql", {
                    { "databases", nlohmann::json::array({ "Main" }) }
                } }
          } }
    }).dump(2);

    initializeDatabaseContext();

    auto runtime = std::make_shared<FakeRuntimeController>();
    AppsManager manager(runtime);

    EXPECT_FALSE(manager.initialize());

    const auto rows = selectAppRow("com.example.invalidpg", { "policy_compile_status", "policy_compile_error" });
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0], "error");
    EXPECT_NE(rows[0][1].find("permissions.postgresql.databases"), std::string::npos);
}

TEST_F(AppsManagerTest, LegacyAppsDbIsBackedUpAndRecreated)
{
    fs::create_directories("data");
    {
        SQLite::Database legacyDb("data/apps.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        legacyDb.exec("CREATE TABLE apps (id INTEGER PRIMARY KEY, app_id TEXT, app_name TEXT, role TEXT, exec_path TEXT, exec_args TEXT);");
    }

    initializeDatabaseContext();

    auto* appsDb = dbManager->getDatabase("apps");
    ASSERT_NE(appsDb, nullptr);
    EXPECT_TRUE(appsDb->columnExists("apps", "manifest_path"));
    EXPECT_FALSE(appsDb->columnExists("apps", "role"));

    bool foundBackup = false;
    for (const auto& entry : fs::directory_iterator("data")) {
        if (entry.path().filename().string().rfind("apps.db.legacy.", 0) == 0) {
            foundBackup = true;
            break;
        }
    }
    EXPECT_TRUE(foundBackup);
}
