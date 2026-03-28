#include "mmWaveSerialLifecycle.h"

MmWaveSerialConnectResult connectMmWaveWithCandidates(
    const MmWaveSerialBackend& backend,
    const std::vector<std::string>& paths,
    const std::vector<int>& baudCandidates,
    int probeTimeoutMs)
{
    MmWaveSerialConnectResult result;

    for (const std::string& path : paths) {
        for (int baud : baudCandidates) {
            MmWaveSerialConnectionAttempt attempt;
            attempt.path = path;
            attempt.baud = baud;

            const int fd = backend.open(path, baud);
            if (fd < 0) {
                result.attempts.push_back(attempt);
                continue;
            }

            attempt.openSucceeded = true;
            attempt.signatureMatched = backend.probeSignature(fd, probeTimeoutMs);
            result.attempts.push_back(attempt);

            if (attempt.signatureMatched) {
                result.fd = fd;
                result.path = path;
                result.baud = baud;
                return result;
            }

            backend.close(fd);
        }
    }

    return result;
}
