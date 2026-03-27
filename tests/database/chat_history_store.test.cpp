#include <gtest/gtest.h>

#include "database/chatHistoryStore.h"
#include "database/cubeDB.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

namespace {

class ChatHistoryStoreTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        originalCwd_ = fs::current_path();
        tempRoot_ = fs::temp_directory_path() / ("chat_history_store_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tempRoot_);
        fs::current_path(tempRoot_);

        dbManager_ = std::make_shared<CubeDatabaseManager>();
        blobsManager_ = std::make_shared<BlobsManager>(dbManager_, "data/blobs.db");
        cubeDb_ = std::make_shared<CubeDB>(dbManager_, blobsManager_);
        store_ = std::make_shared<ChatHistoryStore>(dbManager_);
    }

    void TearDown() override
    {
        CubeDB::setCubeDBManager(nullptr);
        CubeDB::setBlobsManager(nullptr);
        store_.reset();
        cubeDb_.reset();
        blobsManager_.reset();
        dbManager_.reset();
        fs::current_path(originalCwd_);
        fs::remove_all(tempRoot_);
    }

    ChatHistoryEntry makeEntry(
        std::string historyKey,
        std::string requestText,
        std::string displayResponse,
        int64_t createdAtMs,
        std::string intentName = "core.get_time",
        std::string capabilityName = "core.get_time") const
    {
        return ChatHistoryEntry {
            .createdAtMs = createdAtMs,
            .historyKey = std::move(historyKey),
            .intentName = std::move(intentName),
            .capabilityName = std::move(capabilityName),
            .requestText = std::move(requestText),
            .displayResponse = std::move(displayResponse)
        };
    }

    fs::path originalCwd_;
    fs::path tempRoot_;
    std::shared_ptr<CubeDatabaseManager> dbManager_;
    std::shared_ptr<BlobsManager> blobsManager_;
    std::shared_ptr<CubeDB> cubeDb_;
    std::shared_ptr<ChatHistoryStore> store_;
};

} // namespace

TEST_F(ChatHistoryStoreTest, AppendAndReadRecentHistoryByHistoryKey)
{
    ASSERT_TRUE(store_);

    EXPECT_TRUE(store_->appendHistory(makeEntry("core.get_time", "what time is it?", "The clock says 4:45 PM.", 1000)));
    EXPECT_TRUE(store_->appendHistory(makeEntry("core.get_time", "what time is it now?", "It's 5:00 PM right now.", 2000)));
    EXPECT_TRUE(store_->appendHistory(makeEntry("core.get_date", "what date is it?", "Today is Friday.", 1500, "core.get_date", "core.get_date")));

    const auto history = store_->getRecentHistory("core.get_time", 10);
    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].requestText, "what time is it?");
    EXPECT_EQ(history[0].displayResponse, "The clock says 4:45 PM.");
    EXPECT_EQ(history[1].requestText, "what time is it now?");
    EXPECT_EQ(history[1].displayResponse, "It's 5:00 PM right now.");
}

TEST_F(ChatHistoryStoreTest, RetainsOnlyNewestTenRowsPerHistoryKey)
{
    ASSERT_TRUE(store_);

    for (int i = 0; i < 12; ++i) {
        EXPECT_TRUE(store_->appendHistory(makeEntry(
            "core.get_time",
            "request-" + std::to_string(i),
            "response-" + std::to_string(i),
            1000 + i)));
    }
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(store_->appendHistory(makeEntry(
            "core.get_date",
            "date-request-" + std::to_string(i),
            "date-response-" + std::to_string(i),
            2000 + i,
            "core.get_date",
            "core.get_date")));
    }

    const auto timeHistory = store_->getRecentHistory("core.get_time", 10);
    ASSERT_EQ(timeHistory.size(), 10u);
    EXPECT_EQ(timeHistory.front().requestText, "request-2");
    EXPECT_EQ(timeHistory.front().displayResponse, "response-2");
    EXPECT_EQ(timeHistory.back().requestText, "request-11");
    EXPECT_EQ(timeHistory.back().displayResponse, "response-11");

    const auto dateHistory = store_->getRecentHistory("core.get_date", 10);
    ASSERT_EQ(dateHistory.size(), 3u);
    EXPECT_EQ(dateHistory.front().requestText, "date-request-0");
    EXPECT_EQ(dateHistory.back().requestText, "date-request-2");
}
