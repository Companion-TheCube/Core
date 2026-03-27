#include <gtest/gtest.h>

#include "../../src/database/cubeDB.h"

#include <chrono>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

namespace {
class DatabasePredicateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        originalCwd_ = fs::current_path();
        tempRoot_ = fs::temp_directory_path() / ("db_predicate_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tempRoot_);
        fs::current_path(tempRoot_);

        dbManager_ = std::make_shared<CubeDatabaseManager>();
        blobsManager_ = std::make_shared<BlobsManager>(dbManager_, "data/blobs.db");
        cubeDb_ = std::make_shared<CubeDB>(dbManager_, blobsManager_);
    }

    void TearDown() override
    {
        CubeDB::setCubeDBManager(nullptr);
        CubeDB::setBlobsManager(nullptr);
        cubeDb_.reset();
        blobsManager_.reset();
        dbManager_.reset();
        fs::current_path(originalCwd_);
        fs::remove_all(tempRoot_);
    }

    Database* authDb() const
    {
        return dbManager_->getDatabase("auth");
    }

    Database* blobsDb() const
    {
        return dbManager_->getDatabase("blobs");
    }

    fs::path originalCwd_;
    fs::path tempRoot_;
    std::shared_ptr<CubeDatabaseManager> dbManager_;
    std::shared_ptr<BlobsManager> blobsManager_;
    std::shared_ptr<CubeDB> cubeDb_;
};
} // namespace

TEST_F(DatabasePredicateTest, PredicateQueriesTreatInjectedValuesAsData)
{
    auto* db = authDb();
    ASSERT_NE(db, nullptr);

    ASSERT_GT(db->insertData(DB_NS::TableNames::CLIENTS, { "client_id", "initial_code", "auth_code", "role" }, { "victim", "111111", "token-victim", "1" }), 0);
    ASSERT_GT(db->insertData(DB_NS::TableNames::CLIENTS, { "client_id", "initial_code", "auth_code", "role" }, { "other", "222222", "token-other", "1" }), 0);

    const std::string injected = "victim' OR 1=1 --";
    EXPECT_FALSE(db->rowExists(DB_NS::TableNames::CLIENTS, { DB_NS::Predicate { "client_id", injected } }));
    EXPECT_TRUE(db->selectData(DB_NS::TableNames::CLIENTS, { "client_id" }, { DB_NS::Predicate { "client_id", injected } }).empty());

    EXPECT_TRUE(db->updateData(
        DB_NS::TableNames::CLIENTS,
        { "auth_code" },
        { "compromised" },
        { DB_NS::Predicate { "client_id", injected } }));

    const auto victimRow = db->selectData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { DB_NS::Predicate { "client_id", "victim" } });
    const auto otherRow = db->selectData(DB_NS::TableNames::CLIENTS, { "auth_code" }, { DB_NS::Predicate { "client_id", "other" } });
    ASSERT_EQ(victimRow.size(), 1u);
    ASSERT_EQ(otherRow.size(), 1u);
    EXPECT_EQ(victimRow[0][0], "token-victim");
    EXPECT_EQ(otherRow[0][0], "token-other");

    EXPECT_TRUE(db->deleteData(DB_NS::TableNames::CLIENTS, { DB_NS::Predicate { "client_id", injected } }));
    const auto rowsAfterDelete = db->selectData(DB_NS::TableNames::CLIENTS, { "client_id" });
    EXPECT_EQ(rowsAfterDelete.size(), 2u);
}

TEST_F(DatabasePredicateTest, BlobLookupDoesNotBypassOwnerFilterWithInjectedValue)
{
    auto* db = blobsDb();
    ASSERT_NE(db, nullptr);

    const auto blobId = db->insertData(DB_NS::TableNames::CLIENT_BLOBS, { "blob", "blob_size", "owner_client_id" }, { "hello", "5", "owner-1" });
    ASSERT_GT(blobId, 0);

    const auto validBlob = db->selectBlobString(
        DB_NS::TableNames::CLIENT_BLOBS,
        "blob",
        { DB_NS::Predicate { "id", std::to_string(blobId) }, DB_NS::Predicate { "owner_client_id", "owner-1" } });
    EXPECT_EQ(validBlob, "hello");

    const auto injectedBlob = db->selectBlobString(
        DB_NS::TableNames::CLIENT_BLOBS,
        "blob",
        { DB_NS::Predicate { "id", std::to_string(blobId) }, DB_NS::Predicate { "owner_client_id", "owner-1' OR 1=1 --" } });
    EXPECT_TRUE(injectedBlob.empty());
}

TEST_F(DatabasePredicateTest, RejectsUnsafeIdentifiers)
{
    auto* db = authDb();
    ASSERT_NE(db, nullptr);

    EXPECT_FALSE(db->selectData("clients; DROP TABLE clients;", { "client_id" }).size() > 0);
    EXPECT_FALSE(db->rowExists(DB_NS::TableNames::CLIENTS, { DB_NS::Predicate { "client_id; DROP TABLE clients;", "victim" } }));
    EXPECT_FALSE(db->getLastError().empty());
}
