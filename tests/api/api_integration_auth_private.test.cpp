#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

#include "../../src/api/api.h"
#include "../../src/api/authentication.h"
#include "../../src/database/cubeDB.h"
#include "../../src/utils.h"

static void addInterfaceEndpoints(API& api, I_API_Interface& iface)
{
    auto name = iface.getInterfaceName();
    auto endpoints = iface.getHttpEndpointData();
    for (auto& e : endpoints) {
        auto type = std::get<0>(e);
        auto action = std::get<1>(e);
        auto epName = std::get<2>(e);
        api.addEndpoint(name + "-" + epName, "/" + name + "-" + epName, type, action);
    }
}

namespace {
constexpr const char* kAuthRequestsTable = "client_auth_requests";
constexpr int kApiStartupDelayMs = 200;

DB_NS::PredicateList authRequestFilters(const std::string& clientId, const std::string& initialCode)
{
    return {
        DB_NS::Predicate { "client_id", clientId },
        DB_NS::Predicate { "initial_code", initialCode }
    };
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

    explicit ScopedConfigOverride(const std::string& key)
        : key_(key)
        , hadPreviousValue_(Config::has(key))
    {
        if (hadPreviousValue_) {
            previousValue_ = Config::get(key, "");
        }
        Config::erase(key_);
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

class AuthApiIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        namespace fs = std::filesystem;

        prevCwd_ = fs::current_path();
        const auto randSuffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        tmpRoot_ = fs::temp_directory_path() / ("cube_api_test_" + randSuffix);
        fs::create_directories(tmpRoot_);
        fs::current_path(tmpRoot_);

        dbm_ = std::make_shared<CubeDatabaseManager>();
        CubeDB::setCubeDBManager(dbm_);
        blobs_ = std::make_shared<BlobsManager>(dbm_, "data/blobs.db");
        CubeDB::setBlobsManager(blobs_);
        dbm_->openAll();

        api_ = std::make_unique<API>();
        try {
            httpPort_ = std::stoi(Config::get("HTTP_PORT_TEST", "55281"));
        } catch (...) {
            httpPort_ = 55281;
        }

        api_->setHttpBinding("127.0.0.1", httpPort_);
        std::string ipcPath = Config::get("IPC_SOCKET_PATH_TEST", "");
        if (ipcPath.empty()) {
            ipcPath = "test_ipc.sock";
        }
        api_->setIpcPath(ipcPath);

        auth_ = std::make_unique<CubeAuth>();
        addInterfaceEndpoints(*api_, *auth_);

        api_->addEndpoint("Test-privateEcho", testPath(), PRIVATE_ENDPOINT | POST_ENDPOINT,
            [](const httplib::Request& req, httplib::Response& res) {
                nlohmann::json j;
                j["ok"] = true;
                j["payload"] = nlohmann::json::parse(req.body);
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
            });

        api_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(kApiStartupDelayMs));
    }

    void TearDown() override
    {
        namespace fs = std::filesystem;

        if (api_) {
            api_->stop();
        }
        auth_.reset();
        api_.reset();

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

    static std::string testPath()
    {
        return "/Test-privateEcho";
    }

    std::string baseUrl() const
    {
        return "http://127.0.0.1:" + std::to_string(httpPort_);
    }

    Database* authDb() const
    {
        auto* db = CubeDB::getDBManager()->getDatabase("auth");
        EXPECT_NE(db, nullptr);
        EXPECT_TRUE(db != nullptr && db->isOpen());
        return db;
    }

private:
    std::filesystem::path prevCwd_;
    std::filesystem::path tmpRoot_;
    std::shared_ptr<CubeDatabaseManager> dbm_;
    std::shared_ptr<BlobsManager> blobs_;
    std::unique_ptr<API> api_;
    std::unique_ptr<CubeAuth> auth_;
    int httpPort_ = 55281;
};
} // namespace

TEST_F(AuthApiIntegrationTest, PrivateEndpointHonorsResponseBody)
{
    ScopedConfigOverride allowReturnCode("AUTH_ALLOW_RETURN_CODE", "1");
    ScopedConfigOverride autoApproveRequests("AUTH_AUTO_APPROVE_REQUESTS", "1");

    httplib::Client http(baseUrl().c_str());
    const std::string clientID = "test-client-123";

    auto resInit = http.Get(("/CubeAuth-initCode?client_id=" + clientID + "&return_code=true").c_str());
    ASSERT_TRUE(resInit);
    ASSERT_EQ(resInit->status, 200);
    auto jInit = nlohmann::json::parse(resInit->body);
    ASSERT_TRUE(jInit.contains("initial_code"));
    const std::string initialCode = jInit["initial_code"].get<std::string>();

    auto resAuth = http.Get(("/CubeAuth-authHeader?client_id=" + clientID + "&initial_code=" + initialCode).c_str());
    ASSERT_TRUE(resAuth);
    ASSERT_EQ(resAuth->status, 200);
    auto jAuth = nlohmann::json::parse(resAuth->body);
    ASSERT_TRUE(jAuth.contains("auth_code"));
    const std::string token = jAuth["auth_code"].get<std::string>();

    httplib::Headers hdrs = { { "Authorization", std::string("Bearer ") + token }, { "Content-Type", "application/json" } };
    nlohmann::json payload;
    payload["msg"] = "hello";
    auto resTest = http.Post(testPath().c_str(), hdrs, payload.dump(), "application/json");
    ASSERT_TRUE(resTest);
    ASSERT_EQ(resTest->status, 200);
    ASSERT_EQ(resTest->get_header_value("Content-Type"), std::string("application/json"));
    auto jResp = nlohmann::json::parse(resTest->body);
    EXPECT_TRUE(jResp["ok"].get<bool>());
    EXPECT_EQ(jResp["payload"]["msg"].get<std::string>(), "hello");
}

TEST_F(AuthApiIntegrationTest, AuthHeaderFailsWhileApprovalPending)
{
    ScopedConfigOverride allowReturnCode("AUTH_ALLOW_RETURN_CODE", "1");
    ScopedConfigOverride autoApproveRequests("AUTH_AUTO_APPROVE_REQUESTS");

    httplib::Client http(baseUrl().c_str());
    const std::string clientID = "pending-client";

    auto resInit = http.Get(("/CubeAuth-initCode?client_id=" + clientID + "&return_code=true").c_str());
    ASSERT_TRUE(resInit);
    ASSERT_EQ(resInit->status, 200);
    auto jInit = nlohmann::json::parse(resInit->body);
    ASSERT_TRUE(jInit.contains("initial_code"));
    const std::string initialCode = jInit["initial_code"].get<std::string>();

    auto resAuth = http.Get(("/CubeAuth-authHeader?client_id=" + clientID + "&initial_code=" + initialCode).c_str());
    ASSERT_TRUE(resAuth);
    ASSERT_EQ(resAuth->status, 403);
    auto jAuth = nlohmann::json::parse(resAuth->body);
    EXPECT_FALSE(jAuth.value("success", true));
    EXPECT_EQ(jAuth.value("error", std::string()), "approval_pending");
}

TEST_F(AuthApiIntegrationTest, AuthHeaderFailsWhenApprovalDenied)
{
    ScopedConfigOverride allowReturnCode("AUTH_ALLOW_RETURN_CODE", "1");
    ScopedConfigOverride autoApproveRequests("AUTH_AUTO_APPROVE_REQUESTS");

    httplib::Client http(baseUrl().c_str());
    const std::string clientID = "denied-client";

    auto resInit = http.Get(("/CubeAuth-initCode?client_id=" + clientID + "&return_code=true").c_str());
    ASSERT_TRUE(resInit);
    ASSERT_EQ(resInit->status, 200);
    auto jInit = nlohmann::json::parse(resInit->body);
    ASSERT_TRUE(jInit.contains("initial_code"));
    const std::string initialCode = jInit["initial_code"].get<std::string>();

    auto* db = authDb();
    ASSERT_NE(db, nullptr);
    ASSERT_TRUE(db->updateData(
        kAuthRequestsTable,
        { "status", "denied_at_ms" },
        { "denied", "12345" },
        authRequestFilters(clientID, initialCode)));

    const auto rows = db->selectData(kAuthRequestsTable, { "status" }, authRequestFilters(clientID, initialCode));
    ASSERT_EQ(rows.size(), 1u);
    ASSERT_EQ(rows[0][0], "denied");

    auto resAuth = http.Get(("/CubeAuth-authHeader?client_id=" + clientID + "&initial_code=" + initialCode).c_str());
    ASSERT_TRUE(resAuth);
    ASSERT_EQ(resAuth->status, 403);
    auto jAuth = nlohmann::json::parse(resAuth->body);
    EXPECT_FALSE(jAuth.value("success", true));
    EXPECT_EQ(jAuth.value("error", std::string()), "approval_denied");
}

TEST_F(AuthApiIntegrationTest, AuthHeaderFailsWhenApprovalExpired)
{
    ScopedConfigOverride allowReturnCode("AUTH_ALLOW_RETURN_CODE", "1");
    ScopedConfigOverride autoApproveRequests("AUTH_AUTO_APPROVE_REQUESTS");

    httplib::Client http(baseUrl().c_str());
    const std::string clientID = "expired-client";

    auto resInit = http.Get(("/CubeAuth-initCode?client_id=" + clientID + "&return_code=true").c_str());
    ASSERT_TRUE(resInit);
    ASSERT_EQ(resInit->status, 200);
    auto jInit = nlohmann::json::parse(resInit->body);
    ASSERT_TRUE(jInit.contains("initial_code"));
    const std::string initialCode = jInit["initial_code"].get<std::string>();

    auto* db = authDb();
    ASSERT_NE(db, nullptr);
    ASSERT_TRUE(db->updateData(
        kAuthRequestsTable,
        { "status", "expires_at_ms" },
        { "expired", "1" },
        authRequestFilters(clientID, initialCode)));

    const auto rows = db->selectData(kAuthRequestsTable, { "status", "expires_at_ms" }, authRequestFilters(clientID, initialCode));
    ASSERT_EQ(rows.size(), 1u);
    ASSERT_EQ(rows[0][0], "expired");
    ASSERT_EQ(rows[0][1], "1");

    auto resAuth = http.Get(("/CubeAuth-authHeader?client_id=" + clientID + "&initial_code=" + initialCode).c_str());
    ASSERT_TRUE(resAuth);
    ASSERT_EQ(resAuth->status, 403);
    auto jAuth = nlohmann::json::parse(resAuth->body);
    EXPECT_FALSE(jAuth.value("success", true));
    EXPECT_EQ(jAuth.value("error", std::string()), "approval_expired");
}

TEST_F(AuthApiIntegrationTest, AuthHeaderRejectsConsumedApprovalCode)
{
    ScopedConfigOverride allowReturnCode("AUTH_ALLOW_RETURN_CODE", "1");
    ScopedConfigOverride autoApproveRequests("AUTH_AUTO_APPROVE_REQUESTS", "1");

    httplib::Client http(baseUrl().c_str());
    const std::string clientID = "consumed-client";

    auto resInit = http.Get(("/CubeAuth-initCode?client_id=" + clientID + "&return_code=true").c_str());
    ASSERT_TRUE(resInit);
    ASSERT_EQ(resInit->status, 200);
    auto jInit = nlohmann::json::parse(resInit->body);
    ASSERT_TRUE(jInit.contains("initial_code"));
    const std::string initialCode = jInit["initial_code"].get<std::string>();

    auto resAuth = http.Get(("/CubeAuth-authHeader?client_id=" + clientID + "&initial_code=" + initialCode).c_str());
    ASSERT_TRUE(resAuth);
    ASSERT_EQ(resAuth->status, 200);

    auto resAuthReuse = http.Get(("/CubeAuth-authHeader?client_id=" + clientID + "&initial_code=" + initialCode).c_str());
    ASSERT_TRUE(resAuthReuse);
    ASSERT_EQ(resAuthReuse->status, 403);
    auto jReuse = nlohmann::json::parse(resAuthReuse->body);
    EXPECT_FALSE(jReuse.value("success", true));
    EXPECT_EQ(jReuse.value("error", std::string()), "approval_consumed");
}

TEST_F(AuthApiIntegrationTest, InitCodeDoesNotReturnCodeByDefault)
{
    ScopedConfigOverride allowReturnCode("AUTH_ALLOW_RETURN_CODE");
    ScopedConfigOverride autoApproveRequests("AUTH_AUTO_APPROVE_REQUESTS");

    httplib::Client http(baseUrl().c_str());
    const std::string clientID = "no-return-code-client";

    auto resInit = http.Get(("/CubeAuth-initCode?client_id=" + clientID + "&return_code=true").c_str());
    ASSERT_TRUE(resInit);
    ASSERT_EQ(resInit->status, 200);
    auto jInit = nlohmann::json::parse(resInit->body);
    EXPECT_TRUE(jInit.value("success", false));
    EXPECT_FALSE(jInit.contains("initial_code"));
}
