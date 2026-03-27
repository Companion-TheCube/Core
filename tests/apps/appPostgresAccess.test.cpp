#include <gtest/gtest.h>

#include "../src/api/api.h"
#include "../src/apps/appPostgresAccess.h"
#include "../src/database/cubeDB.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

namespace fs = std::filesystem;

namespace {
class FakePostgresAdminClient : public AppPostgresAdminClient {
public:
    bool ensureProvisioned(
        const std::string&,
        const std::string&,
        const std::vector<std::string>&,
        const std::vector<std::string>&,
        std::string&) override
    {
        return true;
    }
};

class AppPostgresAccessTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        originalCwd_ = fs::current_path();
        tempRoot_ = fs::temp_directory_path() / ("app_postgres_access_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tempRoot_);
        fs::current_path(tempRoot_);

        dbManager_ = std::make_shared<CubeDatabaseManager>();
        blobsManager_ = std::make_shared<BlobsManager>(dbManager_, "data/blobs.db");
        cubeDb_ = std::make_shared<CubeDB>(dbManager_, blobsManager_);

        fakeAdmin_ = std::make_shared<FakePostgresAdminClient>();
        AppPostgresAccess::setAdminClientForTests(fakeAdmin_);

        manifest_ = nlohmann::json {
            { "permissions", {
                  { "postgresql", {
                        { "databases", nlohmann::json::array({ "main" }) }
                    } }
              } }
        };
    }

    void TearDown() override
    {
        if (api_ != nullptr) {
            api_->stop();
        }
        AppPostgresAccess::clearAdminClientForTests();
        CubeDB::setCubeDBManager(nullptr);
        CubeDB::setBlobsManager(nullptr);
        cubeDb_.reset();
        blobsManager_.reset();
        dbManager_.reset();
        fs::current_path(originalCwd_);
        fs::remove_all(tempRoot_);
    }

    bool provision(const std::string& appId)
    {
        std::string error;
        const bool ok = AppPostgresAccess::provisionForApp(
            appId,
            manifest_,
            [](const std::string&, std::string&) { return true; },
            error);
        if (!ok) {
            ADD_FAILURE() << error;
        }
        return ok;
    }

    std::string bootstrapToken(const std::string& appId)
    {
        nlohmann::json environmentSet = nlohmann::json::object();
        std::string error;
        EXPECT_TRUE(AppPostgresAccess::appendLaunchEnvironment(appId, { "main" }, environmentSet, error));
        return environmentSet.value("THECUBE_POSTGRES_BOOTSTRAP_TOKEN", "");
    }

    fs::path originalCwd_;
    fs::path tempRoot_;
    std::shared_ptr<CubeDatabaseManager> dbManager_;
    std::shared_ptr<BlobsManager> blobsManager_;
    std::shared_ptr<CubeDB> cubeDb_;
    std::shared_ptr<FakePostgresAdminClient> fakeAdmin_;
    std::unique_ptr<API> api_;
    nlohmann::json manifest_;
};
} // namespace

TEST_F(AppPostgresAccessTest, FetchCredentialsReturnsProvisionedCredentialsForValidBootstrapToken)
{
    const std::string appId = "com.example.dataapp";
    ASSERT_TRUE(provision(appId));

    AppPostgresAccess access;
    const auto endpoints = access.getHttpEndpointData();
    ASSERT_EQ(endpoints.size(), 1u);

    httplib::Request req;
    req.headers.emplace(AppPostgresAccess::bootstrapHeaderName(), bootstrapToken(appId));
    httplib::Response res;

    const auto error = std::get<1>(endpoints[0])(req, res);
    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_EQ(res.status, 200);

    const auto body = nlohmann::json::parse(res.body);
    EXPECT_TRUE(body.value("success", false));
    EXPECT_EQ(body.value("host", std::string()), "127.0.0.1");
    EXPECT_EQ(body.value("port", 0), 5432);
    EXPECT_TRUE(body.contains("username"));
    EXPECT_TRUE(body.contains("password"));
    EXPECT_FALSE(body["databases"]["main"].get<std::string>().empty());
}

TEST_F(AppPostgresAccessTest, FetchCredentialsRejectsInvalidBootstrapToken)
{
    const std::string appId = "com.example.dataapp";
    ASSERT_TRUE(provision(appId));

    AppPostgresAccess access;
    const auto endpoints = access.getHttpEndpointData();
    ASSERT_EQ(endpoints.size(), 1u);

    httplib::Request req;
    req.headers.emplace(AppPostgresAccess::bootstrapHeaderName(), "invalid-token");
    httplib::Response res;

    const auto error = std::get<1>(endpoints[0])(req, res);
    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_EQ(res.status, 403);

    const auto body = nlohmann::json::parse(res.body);
    EXPECT_FALSE(body.value("success", true));
}

TEST_F(AppPostgresAccessTest, FetchCredentialsEndpointIsNotMountedOnHttpServer)
{
    AppPostgresAccess access;
    api_ = std::make_unique<API>();
    api_->setHttpBinding("127.0.0.1", 55282);
    api_->setIpcPath("app_postgres_access_test.sock");
    for (auto& endpoint : access.getHttpEndpointData()) {
        api_->addEndpoint(
            "AppPostgresAccess-" + std::get<2>(endpoint),
            "/AppPostgresAccess-" + std::get<2>(endpoint),
            std::get<0>(endpoint),
            std::get<1>(endpoint));
    }

    api_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    httplib::Client http("127.0.0.1", 55282);
    auto response = http.Post("/AppPostgresAccess-fetchCredentials", "", "application/json");
    ASSERT_TRUE(response);
    EXPECT_EQ(response->status, 404);
}
