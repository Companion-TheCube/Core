/*
███╗   ███╗███╗   ███╗██╗    ██╗ █████╗ ██╗   ██╗███████╗    ██████╗██████╗ ██████╗
████╗ ████║████╗ ████║██║    ██║██╔══██╗██║   ██║██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██╔████╔██║██╔████╔██║██║ █╗ ██║███████║██║   ██║█████╗     ██║     ██████╔╝██████╔╝
██║╚██╔╝██║██║╚██╔╝██║██║███╗██║██╔══██║╚██╗ ██╔╝██╔══╝     ██║     ██╔═══╝ ██╔═══╝
██║ ╚═╝ ██║██║ ╚═╝ ██║╚███╔███╔╝██║  ██║ ╚████╔╝ ███████╗██╗╚██████╗██║     ██║
╚═╝     ╚═╝╚═╝     ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "mmWave.h"
#include "mmWaveSerialLifecycle.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <sstream>
#ifdef __linux__
#include <asm/termbits.h>
#endif

// Serial helpers are implemented later under platform ifdefs.
static int serialDataAvail(int fd);
static uint8_t serialGetchar(int fd);

namespace {
constexpr int MMWAVE_BAUD_PREFERRED = 115200;
constexpr int MMWAVE_BAUD_MANUAL_FALLBACK = 256000;
constexpr int MMWAVE_BAUD_ANALYZER_MEASURED = 263157;
constexpr std::array<const char*, 4> MMWAVE_SERIAL_PATH_CANDIDATES = {
    "/dev/ttyUSB0",
    "/dev/ttyUSB1",
    "/dev/ttyACM0",
    "/dev/ttyACM1"
};

constexpr std::array<int, 7> MMWAVE_BAUD_CANDIDATES = {
    MMWAVE_BAUD_PREFERRED,
    MMWAVE_BAUD_MANUAL_FALLBACK,
    MMWAVE_BAUD_ANALYZER_MEASURED,
    230400,
    460800,
    57600,
    38400
};

constexpr auto MMWAVE_RECONNECT_DELAY = std::chrono::milliseconds(500);

std::string bytesToHexPreview(const std::vector<uint8_t>& bytes, size_t maxBytes = 64)
{
    std::ostringstream oss;
    const size_t limit = std::min(bytes.size(), maxBytes);
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < limit; ++i) {
        oss << std::setw(2) << static_cast<int>(bytes[i]);
        if (i + 1 < limit) {
            oss << " ";
        }
    }
    if (bytes.size() > limit) {
        oss << " ...(" << std::dec << bytes.size() << " bytes total)";
    }
    return oss.str();
}

std::string byteToHex(uint8_t value)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(value);
    return oss.str();
}

bool containsSubsequence(const std::vector<uint8_t>& haystack, const std::vector<uint8_t>& needle)
{
    if (needle.empty()) {
        return true;
    }
    if (haystack.size() < needle.size()) {
        return false;
    }
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end()) != haystack.end();
}

bool hasLikelyFrameSignature(const std::vector<uint8_t>& stream)
{
    const std::vector<uint8_t> reportHeader = REPORT_HEADER;
    return containsSubsequence(stream, reportHeader);
}

bool probeSerialStreamSignature(int fd, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    std::vector<uint8_t> captured;
    captured.reserve(256);

    while (std::chrono::steady_clock::now() < deadline) {
        while (::serialDataAvail(fd) > 0) {
            captured.push_back(::serialGetchar(fd));
            if (captured.size() >= 256) {
                if (hasLikelyFrameSignature(captured)) {
                    return true;
                }
                captured.erase(captured.begin(), captured.begin() + 128);
            }
        }

        if (!captured.empty() && hasLikelyFrameSignature(captured)) {
            return true;
        }
        genericSleep(10);
    }

    CubeLog::debugSilly("mmWave probe: no known frame signature in sample: " + bytesToHexPreview(captured));
    return false;
}
}

#ifdef __linux__
static int serialOpen(const char* port, int baud)
{
    int fd = ::open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        return -1;
    }

    struct termios2 options { };
    if (ioctl(fd, TCGETS2, &options) != 0) {
        ::close(fd);
        return -1;
    }

    // 8N1, no flow control, raw mode.
    options.c_cflag &= ~CBAUD;
    options.c_cflag |= (BOTHER | CS8 | CLOCAL | CREAD);
    options.c_iflag = 0;
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;
    options.c_ispeed = static_cast<unsigned int>(baud);
    options.c_ospeed = static_cast<unsigned int>(baud);

    if (ioctl(fd, TCSETS2, &options) != 0) {
        ::close(fd);
        return -1;
    }

    // Flush input queue after switching baud.
    ioctl(fd, TCFLSH, TCIFLUSH);
    return fd;
}
static int serialDataAvail(int fd)
{
    int bytes = 0;
    ioctl(fd, FIONREAD, &bytes);
    return bytes;
}
static uint8_t serialGetchar(int fd)
{
    uint8_t c = 0;
    ::read(fd, &c, 1);
    return c;
}
static void serialWrite(int fd, const uint8_t* data, size_t len)
{
    ::write(fd, data, len);
}
static void serialClose(int fd)
{
    ::close(fd);
}
static unsigned long millis()
{
    using namespace std::chrono;
    return static_cast<unsigned long>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}
#else
static int serialOpen(const char*, int) { return -1; }
static int serialDataAvail(int) { return 0; }
static uint8_t serialGetchar(int) { return 0; }
static void serialWrite(int, const uint8_t*, size_t) { }
static void serialClose(int) { }
static unsigned long millis() { return 0; }
#endif

namespace {
MmWaveSerialBackend makeRealSerialBackend()
{
    return {
        .open = [](const std::string& path, int baud) { return serialOpen(path.c_str(), baud); },
        .close = [](int fd) { serialClose(fd); },
        .probeSignature = [](int fd, int timeoutMs) { return probeSerialStreamSignature(fd, timeoutMs); }
    };
}
}

void mmWave::readerLoop(std::stop_token stopToken)
{
    unsigned long lastPrintTime = millis();

    while (!stopToken.stop_requested()) {
        bool newlyReady = false;
        bool hasConnection = false;
        {
            std::lock_guard<std::mutex> serialLock(serialMutex);
            if (serialPort_h < 0) {
                const std::vector<std::string> paths(
                    MMWAVE_SERIAL_PATH_CANDIDATES.begin(), MMWAVE_SERIAL_PATH_CANDIDATES.end());
                const std::vector<int> bauds(
                    MMWAVE_BAUD_CANDIDATES.begin(), MMWAVE_BAUD_CANDIDATES.end());
                newlyReady = connectWithCandidatesLocked(paths, bauds, 500);
            }
            hasConnection = (serialPort_h >= 0);
        }

        if (newlyReady) {
            invokeOnReadyCallback();
            lastPrintTime = millis();
            continue;
        }

        if (!hasConnection) {
            genericSleep(static_cast<int>(MMWAVE_RECONNECT_DELAY.count()));
            continue;
        }

        Response response = readDataFrame();
        if (!response.success) {
            markDisconnected("read-loop comms failure: " + response.errorReason);
            genericSleep(static_cast<int>(MMWAVE_RECONNECT_DELAY.count()));
            continue;
        }

        decodeDataFrame(response);

        if (millis() - lastPrintTime > 2000) {
            MmWaveReading r = getReading();
            MmWavePresenceDecision decision = getPresenceDecision();
            CubeLog::debugSilly("Target State: " + std::to_string(r.targetState));
            CubeLog::debugSilly("Moving Target Distance: " + std::to_string(r.movingTargetDistance));
            CubeLog::debugSilly("Moving Target Energy: " + std::to_string(r.movingTargetEnergy));
            CubeLog::debugSilly("Stationary Target Distance: " + std::to_string(r.stationaryTargetDistance));
            CubeLog::debugSilly("Stationary Target Energy: " + std::to_string(r.stationaryTargetEnergy));
            CubeLog::debugSilly("Detection Distance: " + std::to_string(r.detectionDistance));

            decision.state == MmWavePresenceState::Present ? CubeLog::moreInfo("Presence State: PRESENT")
                : decision.state == MmWavePresenceState::Absent     ?  CubeLog::error("Presence State: ABSENT")
                                                        : CubeLog::warning("Presence State: UNKNOWN");
            CubeLog::debugSilly("Averaged Detection Distance: "
                + (decision.detectionDistanceAverageCm.has_value()
                        ? std::to_string(*decision.detectionDistanceAverageCm)
                        : std::string("n/a")));
            CubeLog::debugSilly("Averaged Moving Distance: "
                + (decision.movingTargetDistanceAverageCm.has_value()
                        ? std::to_string(*decision.movingTargetDistanceAverageCm)
                        : std::string("n/a")));
            CubeLog::debugSilly("Averaged Stationary Distance: "
                + (decision.stationaryTargetDistanceAverageCm.has_value()
                        ? std::to_string(*decision.stationaryTargetDistanceAverageCm)
                        : std::string("n/a")));
            CubeLog::debugSilly("Averaged Stationary Energy: "
                + (decision.stationaryTargetEnergyAverage.has_value()
                        ? std::to_string(*decision.stationaryTargetEnergyAverage)
                        : std::string("n/a")));
            CubeLog::debugSilly("Absent Thresholds: detection=" + std::to_string(decision.absentDetectionDistanceThresholdCm)
                + " moving=" + std::to_string(decision.absentMovingDistanceThresholdCm)
                + " stationary=" + std::to_string(decision.absentStationaryDistanceThresholdCm));
            lastPrintTime = millis();
        }

        genericSleep(10);
    }
}

void mmWave::closeSerialPortLocked()
{
    if (serialPort_h >= 0) {
        serialClose(serialPort_h);
        serialPort_h = -1;
    }
    connectedSerialPath_.clear();
    connectedBaud_ = -1;
}

bool mmWave::connectWithCandidatesLocked(const std::vector<std::string>& paths, const std::vector<int>& baudCandidates, int probeTimeoutMs)
{
    closeSerialPortLocked();

    CubeLog::info("mmWave: probing for sensor.");
    const MmWaveSerialConnectResult result = connectMmWaveWithCandidates(
        makeRealSerialBackend(), paths, baudCandidates, probeTimeoutMs);

    for (const MmWaveSerialConnectionAttempt& attempt : result.attempts) {
        if (!attempt.openSucceeded) {
            CubeLog::info("mmWave probe: open failed path=" + attempt.path
                + " baud=" + std::to_string(attempt.baud));
            continue;
        }

        CubeLog::info("mmWave probe: opened path=" + attempt.path
            + " baud=" + std::to_string(attempt.baud));
        if (attempt.signatureMatched) {
            CubeLog::info("mmWave probe: valid frame signature found path=" + attempt.path
                + " baud=" + std::to_string(attempt.baud));
        } else {
            CubeLog::info("mmWave probe: no valid frame signature path=" + attempt.path
                + " baud=" + std::to_string(attempt.baud));
        }
    }

    if (result.fd < 0) {
        isReady_.store(false);
        CubeLog::warning("mmWave: sensor probe failed across all configured paths/bauds.");
        return false;
    }

    serialPort_h = result.fd;
    connectedSerialPath_ = result.path;
    connectedBaud_ = result.baud;
    isReady_.store(true);
    CubeLog::info("mmWave serial ready path=" + connectedSerialPath_
        + " baud=" + std::to_string(connectedBaud_));
    return true;
}

void mmWave::markDisconnected(const std::string& reason)
{
    std::lock_guard<std::mutex> serialLock(serialMutex);
    if (isReady_.exchange(false)) {
        CubeLog::warning("mmWave: connection lost. reason=" + reason);
    } else {
        CubeLog::warning("mmWave: still disconnected. reason=" + reason);
    }
    closeSerialPortLocked();

    std::lock_guard<std::mutex> readingLock(readingMutex);
    resetPresenceStateLocked();
}

void mmWave::resetPresenceStateLocked()
{
    currentReading = { };
    presenceEstimator_.reset();
}

void mmWave::invokeOnReadyCallback()
{
    std::function<void()> callback;
    {
        std::lock_guard<std::mutex> callbackLock(callbackMutex);
        callback = onReadyCallback;
    }
    if (callback) {
        callback();
    }
}

mmWave::mmWave()
{
    this->readerThread = std::make_unique<std::jthread>(
        [this](std::stop_token stopToken) {
            readerLoop(stopToken);
        });
}

mmWave::~mmWave()
{
    if (readerThread) {
        readerThread->request_stop();
        readerThread.reset();
    }

    std::lock_guard<std::mutex> serialLock(serialMutex);
    closeSerialPortLocked();
    isReady_.store(false);

    std::lock_guard<std::mutex> readingLock(readingMutex);
    resetPresenceStateLocked();
}

Response mmWave::readDataFrame()
{
    std::lock_guard<std::mutex> lock(serialMutex);
    Response response;
    if (this->serialPort_h < 0) {
        response.success = false;
        response.errorReason = "serial port unavailable";
        return response;
    }
    response.data = REPORT_HEADER;
    response = true;
    const size_t headerSize = response.size();
    uint16_t waitTime = 0;
    while (serialDataAvail(this->serialPort_h) == 0 && waitTime < 1000) {
        genericSleep(10);
        waitTime += 10;
    }
    if (waitTime >= 1000) {
        CubeLog::error("Timeout trying to get data frame. waitedMs=" + std::to_string(waitTime)
            + ", availNow=" + std::to_string(serialDataAvail(this->serialPort_h)));
        response = false;
        response.errorReason = "data frame timeout";
        return response;
    }
    genericSleep(1);
    if (serialDataAvail(this->serialPort_h) == 0) {
        CubeLog::error("No data available.");
        response = false;
        response.errorReason = "no data available";
        return response;
    }
    int headerIndex = 0;
    std::vector<uint8_t> headerScanPreview;
    headerScanPreview.reserve(32);
    genericSleep(1);
    while (serialDataAvail(this->serialPort_h) > 0) {
        uint8_t c = serialGetchar(this->serialPort_h);
        if (headerScanPreview.size() < 32) {
            headerScanPreview.push_back(c);
        }
        if (c == response[headerIndex]) {
            headerIndex++;
            if (headerIndex == response.size()) {
                break;
            }
        } else {
            headerIndex = 0;
        }
    }
    if (headerIndex != response.size()) {
        CubeLog::error("Failed to find report header. scannedPrefix=" + bytesToHexPreview(headerScanPreview)
            + ", availAfterScan=" + std::to_string(serialDataAvail(this->serialPort_h)));
        response = false;
        response.errorReason = "missing report header";
        return response;
    }

    genericSleep(1);
    uint16_t reportSize = 0;
    reportSize = serialGetchar(this->serialPort_h);
    reportSize |= serialGetchar(this->serialPort_h) << 8;

    response += reportSize & 0xFF;
    response += (reportSize >> 8) & 0xFF;

    if (serialDataAvail(this->serialPort_h) < reportSize) {
        CubeLog::error("Report size does not match data available. reportSize=" + std::to_string(reportSize)
            + ", avail=" + std::to_string(serialDataAvail(this->serialPort_h)));
        response = false;
        response.errorReason = "report size mismatch";
        return response;
    }

    genericSleep(2);
    std::vector<uint8_t> tail = REPORT_TAIL;
    for (uint16_t i = 0; i < reportSize + tail.size(); i++) {
        response += serialGetchar(this->serialPort_h);
    }

    if (response.size() != reportSize + tail.size() + headerSize + 2) { // 2 for the size bytes
        CubeLog::error("Failed to get full report. expectedBytes="
            + std::to_string(reportSize + tail.size() + headerSize + 2)
            + ", actualBytes=" + std::to_string(response.size())
            + ", framePrefix=" + bytesToHexPreview(response.data));
        response = false;
        response.errorReason = "short report";
        return response;
    }

    for (uint16_t i = 1; i <= tail.size(); i++) {
        if (response[response.size() - i] != tail[tail.size() - i]) {
            CubeLog::error("Failed to find report tail. expectedTail=" + bytesToHexPreview(tail)
                + ", frameSuffix="
                + bytesToHexPreview(std::vector<uint8_t>(response.data.end() - tail.size(), response.data.end())));
            response = false;
            response.errorReason = "invalid report tail";
            return response;
        }
    }

    return response;
}

void mmWave::decodeDataFrame(Response response)
{
    if (!response.success) {
        CubeLog::error("Failed to decode data frame.");
        return;
    }
    // CubeLog::debugSilly("Encoded frame: " + response.hexStr);
    std::vector<uint8_t> data = response.getData();

    // datalength, two bytes, little endian
    uint16_t dataLength = data[0] | data[1] << 8;

    // Data type, Normal or Engineering
    uint8_t dataType = data[2];

    // Head
    uint8_t head = data[3];

    if (head != 0xaa) {
        CubeLog::error("Failed to find head. got=0x" + byteToHex(head)
            + ", frame=" + bytesToHexPreview(data));
        return;
    }

    // Tail
    uint8_t tail = data.at(data.size() - 2);

    if (tail != 0x55) {
        CubeLog::error("Failed to find tail. got=0x" + byteToHex(tail)
            + ", frame=" + bytesToHexPreview(data));
        return;
    }

    // check
    uint8_t check = data.at(data.size() - 1);

    if (dataType == 0x02) {
        // CubeLog::info("Normal data frame.");
        if (data.size() != 15) {
            CubeLog::error("Data size does not match. type=normal, size=" + std::to_string(data.size())
                + ", dataLengthField=" + std::to_string(dataLength)
                + ", frame=" + bytesToHexPreview(data));
            return;
        }
        std::lock_guard<std::mutex> lock(readingMutex);
        // target state, 1 byte
        currentReading.targetState = data[4];
        // moving target distance, 2 bytes, cm
        currentReading.movingTargetDistance = data[5] | data[6] << 8;
        // Exercise target energy value, 1 byte
        currentReading.movingTargetEnergy = data[7];
        // Stationary target distance, 2 byte, cm
        currentReading.stationaryTargetDistance = data[8] | data[9] << 8;
        // Stationary target energy, 1 byte
        currentReading.stationaryTargetEnergy = data[10];
        // Detection Distance, 2 byte, cm
        currentReading.detectionDistance = data[11] | data[12] << 8;
        presenceEstimator_.update(currentReading, std::chrono::steady_clock::now());
    } else if (dataType == 0x01) {
        CubeLog::info("Engineering data frame.");
        // Need at least 15 bytes to safely read gate counts at data[13] and data[14]
        if (data.size() < 15) {
            CubeLog::error("Engineering data frame too short. size=" + std::to_string(data.size())
                + ", frame=" + bytesToHexPreview(data));
            return;
        }
        // target state, 1 byte
        {
            std::lock_guard<std::mutex> lock(readingMutex);
            currentReading.targetState = data[4];
            // Moving target distance, 2 bytes, cm
            currentReading.movingTargetDistance = data[5] | data[6] << 8;
            // Moving target energy value, 1 byte
            currentReading.movingTargetEnergy = data[7];
            // Stationary target distance, 2 byte, cm
            currentReading.stationaryTargetDistance = data[8] | data[9] << 8;
            // Stationary target energy, 1 byte
            currentReading.stationaryTargetEnergy = data[10];
            // Detection Distance, 2 byte, cm
            currentReading.detectionDistance = data[11] | data[12] << 8;
            presenceEstimator_.update(currentReading, std::chrono::steady_clock::now());
        }
        // Max moving/stationary gate INDEX N means gates 0..N (N+1 energy values each)
        uint8_t maxMovingDistanceGateNumber = data[13];
        uint8_t maxStationaryDistanceGateNumber = data[14];
        // Expected getData() size: 2 (length bytes) + 13 (fixed fields) +
        // (N_m+1) moving gate energies + (N_s+1) stat gate energies +
        // 1 (photosensitive) + 1 (OUT pin) + 1 (tail 0x55) + 1 (cal 0x00) = 21 + N_m + N_s
        size_t expectedSize = 21 + maxMovingDistanceGateNumber + maxStationaryDistanceGateNumber;
        if (data.size() != expectedSize) {
            CubeLog::error("Engineering data size does not match. size=" + std::to_string(data.size())
                + ", expected=" + std::to_string(expectedSize)
                + ", dataLengthField=" + std::to_string(dataLength)
                + ", frame=" + bytesToHexPreview(data));
            return;
        }
        std::vector<uint8_t> movingDistanceGateEnergy;
        movingDistanceGateEnergy.reserve(maxMovingDistanceGateNumber + 1);
        for (size_t i = 0; i <= maxMovingDistanceGateNumber; i++) {
            movingDistanceGateEnergy.push_back(data[15 + i]);
        }
        std::vector<uint8_t> stationaryDistanceGateEnergy;
        stationaryDistanceGateEnergy.reserve(maxStationaryDistanceGateNumber + 1);
        for (size_t i = 0; i <= maxStationaryDistanceGateNumber; i++) {
            stationaryDistanceGateEnergy.push_back(data[16 + maxMovingDistanceGateNumber + i]);
        }
    } else {
        CubeLog::error("Unknown data type. dataType=0x" + byteToHex(dataType)
            + ", frame=" + bytesToHexPreview(data));
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////

MmWaveReading mmWave::getReading()
{
    std::lock_guard<std::mutex> lock(readingMutex);
    return currentReading;
}

bool mmWave::isPresent()
{
    std::lock_guard<std::mutex> lock(readingMutex);
    return presenceEstimator_.isPresent();
}

MmWavePresenceState mmWave::getPresenceState()
{
    std::lock_guard<std::mutex> lock(readingMutex);
    return presenceEstimator_.state();
}

MmWavePresenceDecision mmWave::getPresenceDecision()
{
    std::lock_guard<std::mutex> lock(readingMutex);
    return presenceEstimator_.decision();
}

void mmWave::setOnReadyCallback(std::function<void()> callback)
{
    {
        std::lock_guard<std::mutex> callbackLock(callbackMutex);
        this->onReadyCallback = std::move(callback);
    }

    if (isReady_.load()) {
        invokeOnReadyCallback();
    }
}

void mmWave::setPresenceConfig(const MmWavePresenceConfig& config)
{
    std::lock_guard<std::mutex> lock(readingMutex);
    presenceEstimator_.setConfig(config);
}

////////////////////////////////////////////////////////////////////////////////////////
