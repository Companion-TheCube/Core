#include "../../src/api/authentication.h"
#include "../../src/api/eventDaemon/eventDaemonService.h"
#include "../../src/database/cubeDB.h"
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

constexpr int kConnectRetryCount = 20;
constexpr int kConnectRetryDelayMs = 50;
constexpr int kReadTimeoutMs = 1000;

bool waitUntil(std::function<bool()> predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return predicate();
}

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

bool writeLine(int fd, const std::string& line)
{
    const std::string payload = line + "\n";
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

std::optional<nlohmann::json> readJsonLine(int fd, int timeoutMs = kReadTimeoutMs)
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

class EventDaemonServiceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        namespace fs = std::filesystem;

        prevCwd_ = fs::current_path();
        tmpRoot_ = fs::temp_directory_path() / ("cube_event_daemon_test_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(tmpRoot_);
        fs::current_path(tmpRoot_);

        dbm_ = std::make_shared<CubeDatabaseManager>();
        CubeDB::setCubeDBManager(dbm_);
        blobs_ = std::make_shared<BlobsManager>(dbm_, "data/blobs.db");
        CubeDB::setBlobsManager(blobs_);
        dbm_->openAll();

        auth_ = std::make_unique<CubeAuth>();
    }

    void TearDown() override
    {
        stopService();

        auth_.reset();
        if (dbm_) {
            dbm_->closeAll();
        }
        blobs_.reset();
        dbm_.reset();

        if (!prevCwd_.empty()) {
            std::filesystem::current_path(prevCwd_);
        }

        std::error_code ec;
        if (!tmpRoot_.empty()) {
            std::filesystem::remove_all(tmpRoot_, ec);
        }
    }

    void startService(size_t brokerHistory = 512, size_t maxQueuedFrames = 128)
    {
        broker_ = std::make_shared<ApiEventBroker>(brokerHistory);
        broker_->registerSource("interaction");
        socketPath_ = (tmpRoot_ / "cube-events.sock").string();
        service_ = std::make_unique<EventDaemonService>(broker_, socketPath_, maxQueuedFrames);
        ASSERT_TRUE(service_->start());
    }

    void stopService()
    {
        if (service_) {
            service_->stop();
        }
        service_.reset();
        broker_.reset();
    }

    void insertApp(const std::string& appId, const std::string& appAuthId)
    {
        auto* db = CubeDB::getDBManager()->getDatabase("apps");
        ASSERT_NE(db, nullptr);
        ASSERT_LT(-1, db->insertData(
                          DB_NS::TableNames::APPS,
                          { "app_id", "app_name", "enabled", "app_auth_id" },
                          { appId, appId, "1", appAuthId }));
    }

    void grantDaemonConnect(const std::string& appId)
    {
        auto* db = CubeDB::getDBManager()->getDatabase("apps");
        ASSERT_NE(db, nullptr);
        if (!db->tableExists("authorized_endpoints")) {
            ASSERT_TRUE(db->createTable(
                "authorized_endpoints",
                { "id", "app_id", "endpoint_name" },
                { "INTEGER PRIMARY KEY", "TEXT", "TEXT" },
                { true, false, false }));
        }
        ASSERT_LT(-1, db->insertData(
                          "authorized_endpoints",
                          { "app_id", "endpoint_name" },
                          { appId, EventDaemonService::kConnectGrantEndpoint }));
    }

    void publishInteractionEvent(uint64_t whenMs, uint64_t sampleSequence)
    {
        broker_->publish(
            "interaction",
            "tap",
            nlohmann::json {
                { "liftStateAfter", "unknown" },
                { "sample", nlohmann::json {
                                  { "xG", static_cast<double>(sampleSequence) },
                                  { "yG", 0.0 },
                                  { "zG", 1.0 },
                              } },
            },
            whenMs);
    }

    std::filesystem::path prevCwd_;
    std::filesystem::path tmpRoot_;
    std::shared_ptr<CubeDatabaseManager> dbm_;
    std::shared_ptr<BlobsManager> blobs_;
    std::unique_ptr<CubeAuth> auth_;
    std::shared_ptr<ApiEventBroker> broker_;
    std::unique_ptr<EventDaemonService> service_;
    std::string socketPath_;
};

TEST_F(EventDaemonServiceTest, AuthorizedClientReceivesReplayAndLiveEvents)
{
    startService();
    insertApp("com.test.app", "app-auth-1");
    grantDaemonConnect("com.test.app");
    publishInteractionEvent(1000, 1);

    const int clientFd = connectUnixSocket(socketPath_);
    ASSERT_GE(clientFd, 0);
    ASSERT_TRUE(writeLine(clientFd, R"({"type":"hello","appAuthId":"app-auth-1","sinceSeq":0,"sources":["interaction"]})"));

    const auto ack = readJsonLine(clientFd);
    ASSERT_TRUE(ack.has_value());
    EXPECT_EQ(ack->value("type", ""), "hello_ack");
    EXPECT_FALSE(ack->value("historyTruncated", true));

    const auto replayEvent = readJsonLine(clientFd);
    ASSERT_TRUE(replayEvent.has_value());
    EXPECT_EQ(replayEvent->value("type", ""), "event");
    EXPECT_EQ((*replayEvent)["event"].value("sequence", 0u), 1u);

    publishInteractionEvent(1200, 2);

    const auto liveEvent = readJsonLine(clientFd);
    ASSERT_TRUE(liveEvent.has_value());
    EXPECT_EQ(liveEvent->value("type", ""), "event");
    EXPECT_EQ((*liveEvent)["event"].value("sequence", 0u), 2u);

    close(clientFd);
    EXPECT_TRUE(waitUntil([&]() { return service_->sessionCount() == 0; }, std::chrono::milliseconds(1500)));
}

TEST_F(EventDaemonServiceTest, UnknownSourceIsRejected)
{
    startService();
    insertApp("com.test.app", "app-auth-2");
    grantDaemonConnect("com.test.app");

    const int clientFd = connectUnixSocket(socketPath_);
    ASSERT_GE(clientFd, 0);
    ASSERT_TRUE(writeLine(clientFd, R"({"type":"hello","appAuthId":"app-auth-2","sinceSeq":0,"sources":["missing"]})"));

    const auto error = readJsonLine(clientFd);
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->value("type", ""), "error");
    EXPECT_EQ(error->value("code", ""), "unknown_source");
    close(clientFd);
}

TEST_F(EventDaemonServiceTest, MissingGrantIsRejected)
{
    startService();
    insertApp("com.test.app", "app-auth-3");

    const int clientFd = connectUnixSocket(socketPath_);
    ASSERT_GE(clientFd, 0);
    ASSERT_TRUE(writeLine(clientFd, R"({"type":"hello","appAuthId":"app-auth-3","sinceSeq":0})"));

    const auto error = readJsonLine(clientFd);
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->value("type", ""), "error");
    EXPECT_EQ(error->value("code", ""), "not_authorized");
    close(clientFd);
}

TEST_F(EventDaemonServiceTest, ReportsHistoryTruncationDuringBootstrap)
{
    startService(3);
    insertApp("com.test.app", "app-auth-4");
    grantDaemonConnect("com.test.app");

    for (uint64_t index = 1; index <= 5; ++index) {
        publishInteractionEvent(1000 + index, index);
    }

    const int clientFd = connectUnixSocket(socketPath_);
    ASSERT_GE(clientFd, 0);
    ASSERT_TRUE(writeLine(clientFd, R"({"type":"hello","appAuthId":"app-auth-4","sinceSeq":0,"sources":["interaction"]})"));

    const auto ack = readJsonLine(clientFd);
    ASSERT_TRUE(ack.has_value());
    EXPECT_TRUE(ack->value("historyTruncated", false));

    const auto event = readJsonLine(clientFd);
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ((*event)["event"].value("sequence", 0u), 3u);
    close(clientFd);
}

TEST_F(EventDaemonServiceTest, SlowReplayClientReceivesResyncRequired)
{
    startService(512, 2);
    insertApp("com.test.app", "app-auth-5");
    grantDaemonConnect("com.test.app");

    for (uint64_t index = 1; index <= 8; ++index) {
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
                { "padding", std::string(2048, 'x') },
            },
            1000 + index);
    }

    const int clientFd = connectUnixSocket(socketPath_);
    ASSERT_GE(clientFd, 0);
    ASSERT_TRUE(writeLine(clientFd, R"({"type":"hello","appAuthId":"app-auth-5","sinceSeq":0,"sources":["interaction"]})"));

    std::optional<nlohmann::json> frame;
    for (int attempt = 0; attempt < 4; ++attempt) {
        frame = readJsonLine(clientFd, 1500);
        if (frame.has_value() && frame->value("type", "") == "error") {
            break;
        }
    }

    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->value("type", ""), "error");
    EXPECT_EQ(frame->value("code", ""), "resync_required");
    close(clientFd);
}

} // namespace
