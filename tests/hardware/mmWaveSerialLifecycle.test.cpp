#include "../../src/hardware/mmWaveSerialLifecycle.h"
#include <gtest/gtest.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
using Key = std::pair<std::string, int>;

struct PairHash {
    size_t operator()(const Key& key) const
    {
        return std::hash<std::string> {}(key.first) ^ (std::hash<int> {}(key.second) << 1);
    }
};
}

TEST(MmWaveSerialLifecycleTest, FallsThroughWrongBaudUntilSignatureMatches)
{
    std::unordered_map<Key, bool, PairHash> signatures = {
        { { "/dev/ttyUSB0", 115200 }, false },
        { { "/dev/ttyUSB0", 256000 }, true }
    };
    std::vector<int> closedFds;
    int nextFd = 10;

    MmWaveSerialBackend backend {
        .open = [&](const std::string& path, int baud) -> int {
            return signatures.count({ path, baud }) ? nextFd++ : -1;
        },
        .close = [&](int fd) {
            closedFds.push_back(fd);
        },
        .probeSignature = [&](int fd, int) -> bool {
            if (fd == 10) {
                return false;
            }
            if (fd == 11) {
                return true;
            }
            return false;
        }
    };

    const MmWaveSerialConnectResult result = connectMmWaveWithCandidates(
        backend, { "/dev/ttyUSB0" }, { 115200, 256000 }, 500);

    ASSERT_EQ(result.attempts.size(), 2u);
    EXPECT_EQ(result.path, "/dev/ttyUSB0");
    EXPECT_EQ(result.baud, 256000);
    EXPECT_EQ(result.fd, 11);
    ASSERT_EQ(closedFds.size(), 1u);
    EXPECT_EQ(closedFds.front(), 10);
}

TEST(MmWaveSerialLifecycleTest, LaterRetryCanSucceedAfterInitialFailure)
{
    bool signatureAvailable = false;
    int nextFd = 20;

    MmWaveSerialBackend backend {
        .open = [&](const std::string& path, int baud) -> int {
            return (path == "/dev/ttyUSB0" && baud == 115200) ? nextFd++ : -1;
        },
        .close = [&](int) {},
        .probeSignature = [&](int, int) -> bool {
            return signatureAvailable;
        }
    };

    const MmWaveSerialConnectResult first = connectMmWaveWithCandidates(
        backend, { "/dev/ttyUSB0" }, { 115200 }, 500);
    EXPECT_EQ(first.fd, -1);

    signatureAvailable = true;
    const MmWaveSerialConnectResult second = connectMmWaveWithCandidates(
        backend, { "/dev/ttyUSB0" }, { 115200 }, 500);

    EXPECT_NE(second.fd, -1);
    EXPECT_EQ(second.path, "/dev/ttyUSB0");
    EXPECT_EQ(second.baud, 115200);
}

TEST(MmWaveSerialLifecycleTest, FallsBackAcrossSerialPaths)
{
    int nextFd = 30;

    MmWaveSerialBackend backend {
        .open = [&](const std::string& path, int baud) -> int {
            return (path == "/dev/ttyACM0" && baud == 115200) ? nextFd++ : -1;
        },
        .close = [&](int) {},
        .probeSignature = [&](int, int) -> bool {
            return true;
        }
    };

    const MmWaveSerialConnectResult result = connectMmWaveWithCandidates(
        backend,
        { "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0" },
        { 115200 },
        500);

    EXPECT_EQ(result.path, "/dev/ttyACM0");
    EXPECT_EQ(result.baud, 115200);
    EXPECT_EQ(result.fd, 30);
    ASSERT_EQ(result.attempts.size(), 3u);
}
