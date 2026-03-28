#pragma once

#include <functional>
#include <string>
#include <vector>

struct MmWaveSerialConnectionAttempt {
    std::string path;
    int baud = -1;
    bool openSucceeded = false;
    bool signatureMatched = false;
};

struct MmWaveSerialConnectResult {
    int fd = -1;
    std::string path;
    int baud = -1;
    std::vector<MmWaveSerialConnectionAttempt> attempts;
};

struct MmWaveSerialBackend {
    std::function<int(const std::string&, int)> open;
    std::function<void(int)> close;
    std::function<bool(int, int)> probeSignature;
};

MmWaveSerialConnectResult connectMmWaveWithCandidates(
    const MmWaveSerialBackend& backend,
    const std::vector<std::string>& paths,
    const std::vector<int>& baudCandidates,
    int probeTimeoutMs);
