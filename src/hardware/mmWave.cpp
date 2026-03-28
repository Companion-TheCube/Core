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
    const std::vector<uint8_t> commandHeader = COMMAND_HEADER;
    return containsSubsequence(stream, reportHeader) || containsSubsequence(stream, commandHeader);
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

    struct termios2 options {};
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
        .open = [](const std::string& path, int baud) {
            return serialOpen(path.c_str(), baud);
        },
        .close = [](int fd) {
            serialClose(fd);
        },
        .probeSignature = [](int fd, int timeoutMs) {
            return probeSerialStreamSignature(fd, timeoutMs);
        }
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
            float score = getPresenceConfidence();
            CubeLog::info("Target State: " + std::to_string(r.targetState));
            CubeLog::info("Moving Target Distance: " + std::to_string(r.movingTargetDistance));
            CubeLog::info("Moving Target Energy: " + std::to_string(r.movingTargetEnergy));
            CubeLog::info("Stationary Target Distance: " + std::to_string(r.stationaryTargetDistance));
            CubeLog::info("Stationary Target Energy: " + std::to_string(r.stationaryTargetEnergy));
            CubeLog::info("Detection Distance: " + std::to_string(r.detectionDistance));
            CubeLog::info("Presence Confidence: " + std::to_string(score));
            lastPrintTime = millis();
            CubeLog::debugSilly("Detected: " + std::string(isPresent() ? "PRESENT" : "ABSENT"));
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
    configModeEnabled = false;
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
    configModeEnabled = false;
    closeSerialPortLocked();

    std::lock_guard<std::mutex> readingLock(readingMutex);
    resetPresenceStateLocked();
}

void mmWave::resetPresenceStateLocked()
{
    currentReading = {};
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
    configModeEnabled = false;
    isReady_.store(false);

    std::lock_guard<std::mutex> readingLock(readingMutex);
    resetPresenceStateLocked();
}

Response mmWave::sendCommand(std::vector<uint8_t> command)
{
    Response response;
    if (this->serialPort_h < 0) {
        response.success = false;
        response.errorReason = "serial port unavailable";
        CubeLog::error("mmWave::sendCommand: serial port unavailable.");
        return response;
    }

    std::vector<uint8_t> dataToSend = COMMAND_HEADER;
    dataToSend.push_back(command.size() & 0xFF);
    dataToSend.push_back((command.size() >> 8) & 0xFF);
    dataToSend.insert(dataToSend.end(), command.begin(), command.end());
    dataToSend.insert(dataToSend.end(), COMMAND_TAIL);

    CubeLog::debugSilly("mmWave::sendCommand: tx payload=" + bytesToHexPreview(command)
        + ", framed=" + bytesToHexPreview(dataToSend)
        + ", port=" + std::to_string(this->serialPort_h));

    serialWrite(this->serialPort_h, dataToSend.data(), dataToSend.size());
    uint16_t waitTime = 0;
    while (serialDataAvail(this->serialPort_h) == 0 && waitTime < 1000) {
        genericSleep(10);
        waitTime += 10;
    }
    if (waitTime >= 1000) {
        CubeLog::error("Timeout trying to get response. cmd=" + bytesToHexPreview(command)
            + ", waitedMs=" + std::to_string(waitTime)
            + ", availNow=" + std::to_string(serialDataAvail(this->serialPort_h)));
        response = false;
        response.errorReason = "command response timeout";
        return response;
    }
    // These nested loops get the data fast, but if we read faster than the data arrives, give  the source
    // time to catch up.
    while (serialDataAvail(this->serialPort_h) > 0) {
        while (serialDataAvail(this->serialPort_h) > 0) {
            response += serialGetchar(this->serialPort_h);
        }
        genericSleep(10);
    }
    response.success = true;
    CubeLog::debugSilly("mmWave::sendCommand: rx=" + bytesToHexPreview(response.data));
    return response;
}

Response mmWave::sendCommand(std::vector<uint8_t> command, std::vector<uint8_t> ack)
{
    Response response = this->sendCommand(command);
    if (!response.success || !containsSubsequence(response.data, ack)) {
        CubeLog::error("Failed to get expected response. cmd=" + bytesToHexPreview(command)
            + ", expectedAck=" + bytesToHexPreview(ack)
            + ", actualRx=" + bytesToHexPreview(response.data)
            + ", rxBytes=" + std::to_string(response.data.size()));
        response = false;
        if (response.errorReason.empty()) {
            response.errorReason = "unexpected command acknowledgement";
        }
        return response;
    }
    return response;
}

bool mmWave::enableConfigMode()
{
    std::vector<uint8_t> commandData = ENABLE_CONFIG_MODE;

    std::vector<uint8_t> ack = COMMAND_HEADER;
    std::vector<uint8_t> ackData = ENABLE_CONFIG_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandData, ack);
    if (response.success) {
        this->configModeEnabled = true;
    } else {
        CubeLog::error("Failed to enable command mode.");
        this->configModeEnabled = false;
    }
    return response.success;
}

bool mmWave::disableConfigMode()
{
    const std::vector<uint8_t> commandData = DISABLE_CONFIG_MODE;

    std::vector<uint8_t> ack = COMMAND_HEADER;
    const std::vector<uint8_t> ackData = DISABLE_CONFIG_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandData, ack);
    // Always clear the flag, even on failure — the sensor may already have exited config mode
    this->configModeEnabled = false;
    return response.success;
}

bool mmWave::enableEngineeringMode()
{
    std::vector<uint8_t> commandData = ENABLE_ENGINEERING_MODE;

    std::vector<uint8_t> ack = COMMAND_HEADER;
    std::vector<uint8_t> ackData = ENABLE_ENGINEERING_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandData, ack);
    return response.success;
}

bool mmWave::disableEngineeringMode()
{
    std::vector<uint8_t> commandData = DISABLE_ENGINEERING_MODE;

    std::vector<uint8_t> ack = COMMAND_HEADER;
    std::vector<uint8_t> ackData = DISABLE_ENGINEERING_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandData, ack);
    return response.success;
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
    int headerSize = response.size();
    if (this->configModeEnabled) {
        CubeLog::error("Command mode enabled. readDataFrame blocked, avail="
            + std::to_string(serialDataAvail(this->serialPort_h)));
        response = false;
        response.errorReason = "config mode enabled";
        return response;
    }
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

float mmWave::getPresenceConfidence()
{
    std::lock_guard<std::mutex> lock(readingMutex);
    return presenceEstimator_.confidence();
}

bool mmWave::isPresent()
{
    std::lock_guard<std::mutex> lock(readingMutex);
    return presenceEstimator_.isOccupied();
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

void mmWave::setPresenceParams(const MmWavePresenceParams& params)
{
    std::lock_guard<std::mutex> lock(readingMutex);
    presenceEstimator_.setParams(params);
}

// ---------------------------------------------------------------------------
// Private helpers — all assume config mode is already active.
// They update the stored config field and send the relevant hardware command.
// ---------------------------------------------------------------------------

// Build a complete ACK frame (COMMAND_HEADER + len(2) + ackData + COMMAND_TAIL)
static std::vector<uint8_t> buildCommandAck(std::vector<uint8_t> ackData)
{
    std::vector<uint8_t> ack = COMMAND_HEADER;
    ack.push_back(static_cast<uint8_t>(ackData.size() & 0xFF));
    ack.push_back(static_cast<uint8_t>((ackData.size() >> 8) & 0xFF));
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    const std::vector<uint8_t> tail = COMMAND_TAIL;
    ack.insert(ack.end(), tail.begin(), tail.end());
    return ack;
}

// Command 0x0060: set distance gate range and unmanned duration (all three params in one frame)
static std::vector<uint8_t> buildDistCmd(int movingGate, int restingGate, int unmannedSecs)
{
    return {
        0x60, 0x00,
        0x00, 0x00, static_cast<uint8_t>(movingGate), 0, 0, 0,
        0x01, 0x00, static_cast<uint8_t>(restingGate), 0, 0, 0,
        0x02, 0x00, static_cast<uint8_t>(unmannedSecs & 0xFF),
        static_cast<uint8_t>((unmannedSecs >> 8) & 0xFF), 0, 0
    };
}

// Command 0x0064: set sensitivity for a gate (0xFFFF = all gates)
static std::vector<uint8_t> buildSensCmd(int gateNum, int motionSens, int restingSens)
{
    return {
        0x64, 0x00,
        0x00, 0x00, static_cast<uint8_t>(gateNum & 0xFF),
        static_cast<uint8_t>((gateNum >> 8) & 0xFF), 0, 0,
        0x01, 0x00, static_cast<uint8_t>(motionSens), 0, 0, 0,
        0x02, 0x00, static_cast<uint8_t>(restingSens), 0, 0, 0
    };
}

bool mmWave::setMaxMovingDistanceGate(int gate)
{
    cfgMovingGate = gate;
    return sendCommand(buildDistCmd(cfgMovingGate, cfgRestingGate, cfgUnmannedSecs),
        buildCommandAck({ 0x60, 0x01, 0x00, 0x00 }))
        .success;
}

bool mmWave::setMaxRestingDistanceGate(int gate)
{
    cfgRestingGate = gate;
    return sendCommand(buildDistCmd(cfgMovingGate, cfgRestingGate, cfgUnmannedSecs),
        buildCommandAck({ 0x60, 0x01, 0x00, 0x00 }))
        .success;
}

bool mmWave::setUnmannedDuration(int seconds)
{
    cfgUnmannedSecs = seconds;
    return sendCommand(buildDistCmd(cfgMovingGate, cfgRestingGate, cfgUnmannedSecs),
        buildCommandAck({ 0x60, 0x01, 0x00, 0x00 }))
        .success;
}

bool mmWave::setDistanceGateSensitivity(int gateNum)
{
    cfgGateNum = gateNum;
    return sendCommand(buildSensCmd(cfgGateNum, cfgMotionSens, cfgRestingSens),
        buildCommandAck({ 0x64, 0x01, 0x00, 0x00 }))
        .success;
}

bool mmWave::setMotionSensitivity(int sensitivity)
{
    cfgMotionSens = sensitivity;
    return sendCommand(buildSensCmd(cfgGateNum, cfgMotionSens, cfgRestingSens),
        buildCommandAck({ 0x64, 0x01, 0x00, 0x00 }))
        .success;
}

bool mmWave::setRestingSensitivity(int sensitivity)
{
    cfgRestingSens = sensitivity;
    return sendCommand(buildSensCmd(cfgGateNum, cfgMotionSens, cfgRestingSens),
        buildCommandAck({ 0x64, 0x01, 0x00, 0x00 }))
        .success;
}

void mmWave::restartModule()
{
    const std::vector<uint8_t> cmd = { 0xA3, 0x00 };
    sendCommand(cmd, buildCommandAck({ 0xA3, 0x01, 0x00, 0x00 }));
    // The sensor restarts after this command; give it time to come back up.
    genericSleep(1000);
}

bool mmWave::configure(int maxMovingGate, int maxRestingGate, int unmannedSecs, int motionSens, int restingSens)
{
    std::lock_guard<std::mutex> lock(serialMutex);
    if (!enableConfigMode()) {
        return false;
    }

    // Cache the new config state
    cfgMovingGate = maxMovingGate;
    cfgRestingGate = maxRestingGate;
    cfgUnmannedSecs = unmannedSecs;
    cfgGateNum = 0xFFFF; // apply to all gates
    cfgMotionSens = motionSens;
    cfgRestingSens = restingSens;

    // One 0x0060 command for distance gates + unmanned duration
    if (!sendCommand(buildDistCmd(maxMovingGate, maxRestingGate, unmannedSecs),
            buildCommandAck({ 0x60, 0x01, 0x00, 0x00 }))
            .success) {
        CubeLog::error("mmWave::configure: failed to set distance gate config.");
        disableConfigMode();
        return false;
    }

    // One 0x0064 command for all-gate sensitivity
    if (!sendCommand(buildSensCmd(0xFFFF, motionSens, restingSens),
            buildCommandAck({ 0x64, 0x01, 0x00, 0x00 }))
            .success) {
        CubeLog::error("mmWave::configure: failed to set sensitivity config.");
        disableConfigMode();
        return false;
    }

    disableConfigMode();
    restartModule();
    return true;
}

bool mmWave::resetToDefaults()
{
    std::lock_guard<std::mutex> lock(serialMutex);
    if (!enableConfigMode()) {
        CubeLog::error("mmWave::resetToDefaults: failed to enter config mode.");
        return false;
    }

    // Avoid hardware factory reset (0xA2): on some modules it restores UART defaults,
    // which can break this session's serial link. Re-apply software defaults instead.
    cfgMovingGate = 5;
    cfgRestingGate = 5;
    cfgUnmannedSecs = 5;
    cfgGateNum = 0xFFFF;
    cfgMotionSens = 50;
    cfgRestingSens = 50;

    const bool distOk = sendCommand(buildDistCmd(cfgMovingGate, cfgRestingGate, cfgUnmannedSecs),
        buildCommandAck({ 0x60, 0x01, 0x00, 0x00 }))
                            .success;
    if (!distOk) {
        CubeLog::error("mmWave::resetToDefaults: failed to apply default distance settings.");
        disableConfigMode();
        return false;
    }

    const bool sensOk = sendCommand(buildSensCmd(cfgGateNum, cfgMotionSens, cfgRestingSens),
        buildCommandAck({ 0x64, 0x01, 0x00, 0x00 }))
                            .success;
    if (!sensOk) {
        CubeLog::error("mmWave::resetToDefaults: failed to apply default sensitivity settings.");
        disableConfigMode();
        return false;
    }

    disableConfigMode();
    restartModule();

    const std::vector<std::string> paths(
        MMWAVE_SERIAL_PATH_CANDIDATES.begin(), MMWAVE_SERIAL_PATH_CANDIDATES.end());

    if (!connectWithCandidatesLocked(
            paths,
            { MMWAVE_BAUD_ANALYZER_MEASURED, MMWAVE_BAUD_MANUAL_FALLBACK, MMWAVE_BAUD_PREFERRED },
            700)) {
        CubeLog::error("mmWave::resetToDefaults: unable to reconnect after restart.");
        return false;
    }

    const int recoveryBaud = connectedBaud_;
    if (recoveryBaud != MMWAVE_BAUD_PREFERRED) {
        if (connectWithCandidatesLocked(paths, { MMWAVE_BAUD_PREFERRED }, 700)) {
            CubeLog::info("mmWave::resetToDefaults: comms settled at preferred baud 115200.");
        } else {
            CubeLog::error("mmWave::resetToDefaults: preferred baud 115200 unavailable; staying at "
                + std::to_string(recoveryBaud));
            if (!connectWithCandidatesLocked(paths, { recoveryBaud }, 700)) {
                CubeLog::error("mmWave::resetToDefaults: failed to restore fallback comms after preferred-baud attempt.");
                return false;
            }
        }
    }

    {
        std::lock_guard<std::mutex> readingLock(readingMutex);
        resetPresenceStateLocked();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////
