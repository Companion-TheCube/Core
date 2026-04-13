#include <gtest/gtest.h>

#include <chrono>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "../../src/api/api.h"
#include "../../src/api/apiEventBroker.h"
#include "../../src/api/authentication.h"
#include "../../src/api/eventDaemon/eventDaemonService.h"
#include "../../src/api/eventsAPI.h"
#include "../../src/database/cubeDB.h"
#include "../../src/hardware/interactionAPI.h"
#include "../../src/hardware/peripheralManager.h"
#include "../../src/settings/globalSettings.h"
#include "../../src/utils.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void addInterfaceEndpoints(API& api, I_API_Interface& iface)
{
    const auto name = iface.getInterfaceName();
    const auto endpoints = iface.getHttpEndpointData();
    for (const auto& endpoint : endpoints) {
        api.addEndpoint(
            name + "-" + std::get<2>(endpoint),
            "/" + name + "-" + std::get<2>(endpoint),
            std::get<0>(endpoint),
            std::get<1>(endpoint));
    }
}

namespace {

constexpr const char* kIpcAppAuthHeaderName = "X-TheCube-App-Auth-Id";
constexpr int kApiStartupDelayMs = 200;
constexpr int kConnectRetryCount = 20;
constexpr int kConnectRetryDelayMs = 50;
constexpr int kSocketReadTimeoutMs = 1500;

int connectUnixSocket(const std::string& socketPath)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    for (int attempt = 0; attempt < kConnectRetryCount; ++attempt) {
        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            return fd;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kConnectRetryDelayMs));
    }

    close(fd);
    return -1;
}

bool writeJsonLine(int fd, const nlohmann::json& json)
{
    const std::string payload = json.dump() + "\n";
    size_t totalWritten = 0;
    while (totalWritten < payload.size()) {
        const ssize_t written = send(fd, payload.data() + totalWritten, payload.size() - totalWritten, MSG_NOSIGNAL);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (written == 0) {
            return false;
        }
        totalWritten += static_cast<size_t>(written);
    }
    return true;
}

std::optional<nlohmann::json> readJsonLine(int fd, int timeoutMs = kSocketReadTimeoutMs)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);

    timeval timeout {};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    const int ready = select(fd + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready <= 0 || !FD_ISSET(fd, &readSet)) {
        return std::nullopt;
    }

    std::string line;
    char ch = '\0';
    while (true) {
        const ssize_t bytesRead = recv(fd, &ch, 1, 0);
        if (bytesRead <= 0) {
            return std::nullopt;
        }
        if (ch == '\n') {
            break;
        }
        if (ch != '\r') {
            line.push_back(ch);
        }
    }

    return nlohmann::json::parse(line);
}

class ScopedConfigOverride {
public:
    ScopedConfigOverride(const std::string& key, const std::string& value)
        : key_(key)
        , hadPreviousValue_(Config::has(key))
    {
        if (hadPreviousValue_) {
            previousValue_ = Config::get(key, "");
        }
        Config::set(key_, value);
    }

    ~ScopedConfigOverride()
    {
        if (hadPreviousValue_) {
            Config::set(key_, previousValue_);
        } else {
            Config::erase(key_);
        }
    }

private:
    std::string key_;
    bool hadPreviousValue_ = false;
    std::string previousValue_;
};

class EventsApiIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        namespace fs = std::filesystem;

        prevCwd_ = fs::current_path();
        tmpRoot_ = fs::temp_directory_path() / ("cube_events_api_test_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tmpRoot_);
        std::error_code symlinkEc;
        fs::create_directory_symlink(prevCwd_ / "fonts", tmpRoot_ / "fonts", symlinkEc);
        fs::current_path(tmpRoot_);

        dbm_ = std::make_shared<CubeDatabaseManager>();
        CubeDB::setCubeDBManager(dbm_);
        blobs_ = std::make_shared<BlobsManager>(dbm_, "data/blobs.db");
        CubeDB::setBlobsManager(blobs_);
        dbm_->openAll();
        settings_ = std::make_unique<GlobalSettings>();

        api_ = std::make_shared<API>();
        try {
            httpPort_ = std::stoi(Config::get("HTTP_PORT_TEST", "55281"));
        } catch (...) {
            httpPort_ = 55281;
        }

        api_->setHttpBinding("127.0.0.1", httpPort_);

        std::string ipcPath = Config::get("IPC_SOCKET_PATH_TEST", "test_events_ipc.sock");
        std::filesystem::path resolvedIpcPath = ipcPath;
        if (!resolvedIpcPath.is_absolute()) {
            resolvedIpcPath = tmpRoot_ / resolvedIpcPath;
        }
        ipcPath_ = resolvedIpcPath.lexically_normal().string();
        api_->setIpcPath(ipcPath_);

        auth_ = std::make_unique<CubeAuth>();
        broker_ = api_->getEventBroker();
        broker_->registerSource("interaction");
        eventsApi_ = std::make_unique<EventsAPI>(api_);
        peripheralManager_ = std::make_shared<PeripheralManager>(
            nullptr,
            nullptr,
            nullptr,
            PeripheralManager::TemperatureReader {},
            PeripheralManager::TemperatureReader {},
            PeripheralManager::MonotonicNowReader {},
            PeripheralManager::EpochMsReader {},
            false,
            false,
            false);
        interactionApi_ = std::make_unique<InteractionAPI>(peripheralManager_, broker_);

        addInterfaceEndpoints(*api_, *auth_);
        addInterfaceEndpoints(*api_, *eventsApi_);
        addInterfaceEndpoints(*api_, *interactionApi_);

        api_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(kApiStartupDelayMs));
    }

    void TearDown() override
    {
        namespace fs = std::filesystem;

        if (api_) {
            api_->stop();
        }

        interactionApi_.reset();
        peripheralManager_.reset();
        eventsApi_.reset();
        broker_.reset();
        auth_.reset();
        api_.reset();
        settings_.reset();

        if (dbm_) {
            dbm_->closeAll();
        }
        blobs_.reset();
        dbm_.reset();

        if (!prevCwd_.empty()) {
            fs::current_path(prevCwd_);
        }

        std::error_code ec;
        if (!tmpRoot_.empty()) {
            fs::remove_all(tmpRoot_, ec);
        }
    }

    std::string baseUrl() const
    {
        return "http://127.0.0.1:" + std::to_string(httpPort_);
    }

    Database* appsDb() const
    {
        auto* db = CubeDB::getDBManager()->getDatabase("apps");
        EXPECT_NE(db, nullptr);
        return db;
    }

    void insertApp(const std::string& appId, const std::string& appName, const std::string& appAuthId)
    {
        auto* db = appsDb();
        ASSERT_NE(db, nullptr);
        ASSERT_LT(-1, db->insertData(
                          DB_NS::TableNames::APPS,
                          { "app_id", "app_name", "enabled", "app_auth_id" },
                          { appId, appName, "1", appAuthId }));
    }

    void ensureAuthorizedEndpointsTable()
    {
        auto* db = appsDb();
        ASSERT_NE(db, nullptr);
        if (!db->tableExists("authorized_endpoints")) {
            ASSERT_TRUE(db->createTable(
                "authorized_endpoints",
                { "id", "app_id", "endpoint_name" },
                { "INTEGER PRIMARY KEY", "TEXT", "TEXT" },
                { true, false, false }));
        }
    }

    void grantEndpoint(const std::string& appId, const std::string& endpointName)
    {
        ensureAuthorizedEndpointsTable();
        auto* db = appsDb();
        ASSERT_NE(db, nullptr);
        ASSERT_LT(-1, db->insertData(
                          "authorized_endpoints",
                          { "app_id", "endpoint_name" },
                          { appId, endpointName }));
    }

    std::string issueBearerToken()
    {
        ScopedConfigOverride allowReturnCode("AUTH_ALLOW_RETURN_CODE", "1");
        ScopedConfigOverride autoApprove("AUTH_AUTO_APPROVE_REQUESTS", "1");

        httplib::Client http(baseUrl().c_str());
        const std::string clientId = "events-api-http-client";

        auto initRes = http.Get(("/CubeAuth-initCode?client_id=" + clientId + "&return_code=true").c_str());
        EXPECT_TRUE(initRes);
        if (!initRes || initRes->status != 200) {
            return {};
        }
        EXPECT_EQ(initRes->status, 200);
        const auto initJson = nlohmann::json::parse(initRes->body);
        if (!initJson.contains("initial_code")) {
            return {};
        }

        auto authRes = http.Get(("/CubeAuth-authHeader?client_id=" + clientId + "&initial_code="
                                 + initJson["initial_code"].get<std::string>())
                                    .c_str());
        EXPECT_TRUE(authRes);
        if (!authRes || authRes->status != 200) {
            return {};
        }
        EXPECT_EQ(authRes->status, 200);
        const auto authJson = nlohmann::json::parse(authRes->body);
        if (!authJson.contains("auth_code")) {
            return {};
        }
        return authJson["auth_code"].get<std::string>();
    }

    std::filesystem::path prevCwd_;
    std::filesystem::path tmpRoot_;
    std::shared_ptr<CubeDatabaseManager> dbm_;
    std::shared_ptr<BlobsManager> blobs_;
    std::shared_ptr<API> api_;
    std::unique_ptr<CubeAuth> auth_;
    std::shared_ptr<ApiEventBroker> broker_;
    std::unique_ptr<EventsAPI> eventsApi_;
    std::shared_ptr<PeripheralManager> peripheralManager_;
    std::unique_ptr<InteractionAPI> interactionApi_;
    std::unique_ptr<GlobalSettings> settings_;
    int httpPort_ = 55281;
    std::string ipcPath_;
};

TEST_F(EventsApiIntegrationTest, HttpPrivateEventsEndpointReturnsPublishedInteractionEvent)
{
    const std::string token = issueBearerToken();
    ASSERT_FALSE(token.empty());

    broker_->publish(
        "interaction",
        "tap",
        nlohmann::json {
            { "liftStateAfter", "unknown" },
            { "sample", nullptr },
        },
        1000);

    httplib::Client http(baseUrl().c_str());
    httplib::Headers headers = {
        { "Authorization", std::string("Bearer ") + token },
    };

    auto res = http.Get("/Events-wait?since_seq=0&sources=interaction&limit=10&wait_ms=0", headers);
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);

    const auto body = nlohmann::json::parse(res->body);
    ASSERT_TRUE(body.contains("events"));
    ASSERT_EQ(body["events"].size(), 1u);
    EXPECT_EQ(body["events"][0].value("source", ""), "interaction");
    EXPECT_EQ(body["events"][0].value("event", ""), "tap");
}

TEST_F(EventsApiIntegrationTest, IpcGrantedAppCanReadUnifiedAndCompatibilityInteractionEndpoints)
{
    const std::string appId = "test.app";
    const std::string appAuthId = "runtime-auth-123";
    insertApp(appId, "Test App", appAuthId);
    grantEndpoint(appId, "Events-wait");
    grantEndpoint(appId, "Interaction-events");
    grantEndpoint(appId, "Interaction-status");

    broker_->publish(
        "interaction",
        "lift_started",
        nlohmann::json {
            { "liftStateAfter", "lifted" },
            { "sample", nlohmann::json { { "xG", 0.4 }, { "yG", 0.0 }, { "zG", 0.7 } } },
        },
        1500);

    httplib::Client ipc(ipcPath_.c_str(), 0);
    ipc.set_address_family(AF_UNIX);
    const httplib::Headers headers = {
        { kIpcAppAuthHeaderName, appAuthId },
    };

    auto unifiedRes = ipc.Get("/Events-wait?since_seq=0&sources=interaction&limit=10&wait_ms=0", headers);
    ASSERT_TRUE(unifiedRes);
    ASSERT_EQ(unifiedRes->status, 200);

    const auto unifiedBody = nlohmann::json::parse(unifiedRes->body);
    ASSERT_TRUE(unifiedBody.contains("events"));
    ASSERT_EQ(unifiedBody["events"].size(), 1u);
    EXPECT_EQ(unifiedBody["events"][0].value("event", ""), "lift_started");

    auto compatibilityRes = ipc.Get("/Interaction-events?since_seq=0&limit=10", headers);
    ASSERT_TRUE(compatibilityRes);
    ASSERT_EQ(compatibilityRes->status, 200);

    const auto compatibilityBody = nlohmann::json::parse(compatibilityRes->body);
    ASSERT_TRUE(compatibilityBody.contains("events"));
    ASSERT_EQ(compatibilityBody["events"].size(), 1u);
    EXPECT_EQ(compatibilityBody["events"][0].value("type", ""), "lift_started");
    EXPECT_EQ(compatibilityBody["events"][0].value("liftStateAfter", ""), "lifted");

    auto statusRes = ipc.Get("/Interaction-status", headers);
    ASSERT_TRUE(statusRes);
    ASSERT_EQ(statusRes->status, 200);

    const auto statusBody = nlohmann::json::parse(statusRes->body);
    EXPECT_TRUE(statusBody.contains("available"));
    EXPECT_TRUE(statusBody.contains("enabled"));
    EXPECT_TRUE(statusBody.contains("initialized"));
    EXPECT_TRUE(statusBody.contains("lastEventSequence"));
}

TEST_F(EventsApiIntegrationTest, ManyLocalDaemonClientsDoNotBlockHttpEventsEndpoint)
{
    constexpr int kClientCount = 30;

    const std::string daemonSocketPath = api_->getResolvedEventDaemonPath();
    ASSERT_FALSE(daemonSocketPath.empty());

    std::vector<int> clientFds;
    clientFds.reserve(kClientCount);

    for (int index = 0; index < kClientCount; ++index) {
        const std::string appId = "daemon.app." + std::to_string(index);
        const std::string appAuthId = "daemon-auth-" + std::to_string(index);
        insertApp(appId, "Daemon Test App " + std::to_string(index), appAuthId);
        grantEndpoint(appId, EventDaemonService::kConnectGrantEndpoint);

        const int clientFd = connectUnixSocket(daemonSocketPath);
        ASSERT_GE(clientFd, 0);
        clientFds.push_back(clientFd);

        ASSERT_TRUE(writeJsonLine(clientFd, nlohmann::json {
                                                { "type", "hello" },
                                                { "appAuthId", appAuthId },
                                                { "sinceSeq", 0 },
                                                { "sources", nlohmann::json::array({ "interaction" }) },
                                            }));

        const auto ack = readJsonLine(clientFd);
        ASSERT_TRUE(ack.has_value());
        EXPECT_EQ(ack->value("type", ""), "hello_ack");
        EXPECT_FALSE(ack->value("historyTruncated", true));
    }

    const std::string token = issueBearerToken();
    ASSERT_FALSE(token.empty());

    broker_->publish(
        "interaction",
        "tap",
        nlohmann::json {
            { "liftStateAfter", "unknown" },
            { "sample", nlohmann::json {
                              { "xG", 0.0 },
                              { "yG", 0.0 },
                              { "zG", 1.0 },
                          } },
        },
        2000);

    for (const int index : { 0, kClientCount / 2, kClientCount - 1 }) {
        const auto eventFrame = readJsonLine(clientFds.at(static_cast<size_t>(index)));
        ASSERT_TRUE(eventFrame.has_value());
        EXPECT_EQ(eventFrame->value("type", ""), "event");
        ASSERT_TRUE(eventFrame->contains("event"));
        EXPECT_EQ((*eventFrame)["event"].value("source", ""), "interaction");
        EXPECT_EQ((*eventFrame)["event"].value("event", ""), "tap");
    }

    httplib::Client http(baseUrl().c_str());
    const httplib::Headers headers = {
        { "Authorization", std::string("Bearer ") + token },
    };

    auto res = http.Get("/Events-wait?since_seq=0&sources=interaction&limit=10&wait_ms=0", headers);
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);

    const auto body = nlohmann::json::parse(res->body);
    ASSERT_TRUE(body.contains("events"));
    ASSERT_EQ(body["events"].size(), 1u);
    EXPECT_EQ(body["events"][0].value("source", ""), "interaction");
    EXPECT_EQ(body["events"][0].value("event", ""), "tap");

    for (const int clientFd : clientFds) {
        close(clientFd);
    }
}

} // namespace
