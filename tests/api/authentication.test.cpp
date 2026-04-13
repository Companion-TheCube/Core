#include <gtest/gtest.h>

#include "../../src/api/authentication.h"
#include "../../src/database/cubeDB.h"

#include <chrono>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

namespace {
class AuthAppAuthorizationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        previousCwd_ = fs::current_path();
        tempRoot_ = fs::temp_directory_path() / ("cube_auth_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tempRoot_ / "data");
        fs::current_path(tempRoot_);

        dbManager_ = std::make_shared<CubeDatabaseManager>();
        CubeDB::setCubeDBManager(dbManager_);
        blobsManager_ = std::make_shared<BlobsManager>(dbManager_, "data/blobs.db");
        CubeDB::setBlobsManager(blobsManager_);
        dbManager_->openAll();

        auth_ = std::make_unique<CubeAuth>();
    }

    void TearDown() override
    {
        auth_.reset();
        if (dbManager_ != nullptr) {
            dbManager_->closeAll();
        }
        CubeDB::setBlobsManager(nullptr);
        CubeDB::setCubeDBManager(nullptr);
        blobsManager_.reset();
        dbManager_.reset();

        fs::current_path(previousCwd_);
        std::error_code ec;
        fs::remove_all(tempRoot_, ec);
    }

    Database* appsDb() const
    {
        auto* db = CubeDB::getDBManager()->getDatabase("apps");
        EXPECT_NE(db, nullptr);
        EXPECT_TRUE(db != nullptr && db->isOpen());
        return db;
    }

    void insertApp(const std::string& appId, const std::string& appName, const std::string& appAuthId, const std::string& enabled = "1")
    {
        auto* db = appsDb();
        ASSERT_NE(db, nullptr);
        ASSERT_LT(-1, db->insertData(
                          DB_NS::TableNames::APPS,
                          { "app_id", "app_name", "enabled", "app_auth_id" },
                          { appId, appName, enabled, appAuthId }));
    }

    void ensureAuthorizedEndpointsTableForTest()
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
        ensureAuthorizedEndpointsTableForTest();
        auto* db = appsDb();
        ASSERT_NE(db, nullptr);
        ASSERT_LT(-1, db->insertData(
                          "authorized_endpoints",
                          { "app_id", "endpoint_name" },
                          { appId, endpointName }));
    }

private:
    fs::path previousCwd_;
    fs::path tempRoot_;
    std::shared_ptr<CubeDatabaseManager> dbManager_;
    std::shared_ptr<BlobsManager> blobsManager_;
    std::unique_ptr<CubeAuth> auth_;
};
} // namespace

TEST_F(AuthAppAuthorizationTest, IsAuthorizedAppDeniesMissingAppAuthId)
{
    EXPECT_FALSE(CubeAuth::isAuthorizedApp("", "GUI-messageBox"));
    EXPECT_EQ(CubeAuth::getLastError(), "App auth ID is required.");
}

TEST_F(AuthAppAuthorizationTest, IsAuthorizedAppDeniesUnknownAppAuthId)
{
    EXPECT_FALSE(CubeAuth::isAuthorizedApp("missing-auth-id", "GUI-messageBox"));
    EXPECT_EQ(CubeAuth::getLastError(), "App auth ID not found.");
}

TEST_F(AuthAppAuthorizationTest, IsAuthorizedAppDeniesKnownAppWithoutGrantAndCreatesTable)
{
    insertApp("com.example.clock", "Clock", "clock-auth-id");

    EXPECT_FALSE(CubeAuth::isAuthorizedApp("clock-auth-id", "GUI-messageBox"));
    EXPECT_EQ(CubeAuth::getLastError(), "App is not authorized to access endpoint.");
    ASSERT_NE(appsDb(), nullptr);
    EXPECT_TRUE(appsDb()->tableExists("authorized_endpoints"));
}

TEST_F(AuthAppAuthorizationTest, IsAuthorizedAppAllowsKnownAppWithMatchingGrant)
{
    insertApp("com.example.clock", "Clock", "clock-auth-id");
    grantEndpoint("com.example.clock", "GUI-messageBox");

    EXPECT_TRUE(CubeAuth::isAuthorizedApp("clock-auth-id", "GUI-messageBox"));
    EXPECT_EQ(CubeAuth::getLastError(), "");
}

TEST_F(AuthAppAuthorizationTest, IsAuthorizedAppDeniesStaleAppAuthIdAfterRotation)
{
    insertApp("com.example.clock", "Clock", "clock-auth-id");
    grantEndpoint("com.example.clock", "GUI-messageBox");

    EXPECT_TRUE(CubeAuth::isAuthorizedApp("clock-auth-id", "GUI-messageBox"));

    auto* db = appsDb();
    ASSERT_NE(db, nullptr);
    ASSERT_TRUE(db->updateData(
        DB_NS::TableNames::APPS,
        { "app_auth_id" },
        { "clock-auth-id-rotated" },
        { DB_NS::Predicate { "app_id", "com.example.clock" } }));

    EXPECT_FALSE(CubeAuth::isAuthorizedApp("clock-auth-id", "GUI-messageBox"));
    EXPECT_EQ(CubeAuth::getLastError(), "App auth ID not found.");

    EXPECT_TRUE(CubeAuth::isAuthorizedApp("clock-auth-id-rotated", "GUI-messageBox"));
    EXPECT_EQ(CubeAuth::getLastError(), "");
}

TEST(CubeDBGlobalsTest, NullManagersAreTreatedAsUnset)
{
    CubeDB::setBlobsManager(nullptr);
    CubeDB::setCubeDBManager(nullptr);

    EXPECT_THROW((void)CubeDB::getDBManager(), std::runtime_error);
    EXPECT_THROW((void)CubeDB::getBlobsManager(), std::runtime_error);
}
