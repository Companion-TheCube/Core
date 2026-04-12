/*
███╗   ███╗███╗   ███╗██╗    ██╗ █████╗ ██╗   ██╗███████╗   ██╗  ██╗
████╗ ████║████╗ ████║██║    ██║██╔══██╗██║   ██║██╔════╝   ██║  ██║
██╔████╔██║██╔████╔██║██║ █╗ ██║███████║██║   ██║█████╗     ███████║
██║╚██╔╝██║██║╚██╔╝██║██║███╗██║██╔══██║╚██╗ ██╔╝██╔══╝     ██╔══██║
██║ ╚═╝ ██║██║ ╚═╝ ██║╚███╔███╔╝██║  ██║ ╚████╔╝ ███████╗██╗██║  ██║
╚═╝     ╚═╝╚═╝     ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

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

#pragma once
#include "mmWavePresenceEstimator.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#ifndef LOGGER_H
#include <logger.h>
#endif
#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

// TODO: add some ifdefs and defines for the port name on RasPi
// TODO: add ifdefs for Windows so that this will compile on Windows
// For windows, we should just mock the mmWave sensor.

#define REPORT_HEADER { 0xF4, 0xF3, 0xF2, 0xF1 }
#define REPORT_TAIL { 0xF8, 0xF7, 0xF6, 0xF5 }

class Response {
public:
    bool success = false;
    std::vector<uint8_t> data = {};
    std::string hexStr = "";
    std::string errorReason = "";

    std::vector<uint8_t> getHeader()
    {
        return { data[0], data[1], data[2], data[3] };
    }

    std::vector<uint8_t> getTail()
    {
        return { data[data.size() - 4], data[data.size() - 3], data[data.size() - 2], data[data.size() - 1] };
    }

    std::vector<uint8_t> getData()
    {
        return { data.begin() + 4, data.end() - 4 };
    }

    Response() = default;

    size_t size()
    {
        return data.size();
    }

    uint8_t operator[](const size_t& index)
    {
        if (index < data.size()) {
            return data.at(index);
        }
        return 0;
    }

    bool operator==(const std::vector<uint8_t>& other)
    {
        bool equal = false;
        if (data.size() == other.size()) {
            equal = true;
            for (size_t i = 0; i < data.size(); i++) {
                if (data[i] != other[i]) {
                    equal = false;
                    break;
                }
            }
        }
        return equal;
    }

    bool operator!=(const std::vector<uint8_t>& other)
    {
        return !(*this == other);
    }

    bool operator==(const bool& other)
    {
        return success == other;
    }

    Response& operator=(const bool& newSuccess)
    {
        success = newSuccess;
        return *this;
    }

    Response& operator=(const std::vector<uint8_t>& newData)
    {
        data = newData;
        updateHexStr();
        return *this;
    }

    Response& operator+=(const uint8_t& extraData)
    {
        data.push_back(extraData);
        updateHexStr();
        return *this;
    }

    Response& operator+=(const char& extraData)
    {
        data.push_back(static_cast<uint8_t>(extraData));
        updateHexStr();
        return *this;
    }

    // Addition operator
    Response& operator+=(const std::vector<uint8_t>& extraData)
    {
        data.insert(data.end(), extraData.begin(), extraData.end());
        updateHexStr();
        return *this;
    }

    // Addition operator for adding two Response objects' data together
    Response& operator+=(const Response& other)
    {
        data.insert(data.end(), other.data.begin(), other.data.end());
        updateHexStr();
        success = success && other.success;
        return *this;
    }

    Response& operator+=(const int& extraData)
    {
        data.push_back(static_cast<uint8_t>(extraData));
        updateHexStr();
        return *this;
    }

    // Addition operator for creating a new Response object as the sum of two others
    Response operator+(const Response& other) const
    {
        Response result = *this;
        result += other;
        result.success = success && other.success;
        return result;
    }

private:
    // Helper function to convert vector data to hex string
    void updateHexStr()
    {
        std::ostringstream oss;
        for (const auto& byte : data) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        hexStr = oss.str();
    }
};

struct MmWaveReading {
    uint8_t targetState = 0;
    uint16_t movingTargetDistance = 0;
    uint8_t movingTargetEnergy = 0;
    uint16_t stationaryTargetDistance = 0;
    uint8_t stationaryTargetEnergy = 0;
    uint16_t detectionDistance = 0;
};

class mmWave {
    // Current reading protected by readingMutex
    std::mutex readingMutex;
    MmWaveReading currentReading;
    MmWavePresenceEstimator presenceEstimator_;
    bool presenceDetectionEnabled_ = true;

    std::mutex serialMutex;
    std::mutex callbackMutex;

    size_t connectionFailureCount = 0;

    int serialPort_h = -1;
    std::atomic<bool> isReady_ { false };
    std::function<void()> onReadyCallback;
    std::function<void(const MmWavePresenceDecision&)> onPresenceUpdateCallback;
    std::unique_ptr<std::jthread> readerThread;
    std::string connectedSerialPath_;
    int connectedBaud_ = -1;

    Response readDataFrame();
    void decodeDataFrame(Response response);
    void readerLoop(std::stop_token stopToken);
    void closeSerialPortLocked();
    bool connectWithCandidatesLocked(const std::vector<std::string>& paths, const std::vector<int>& baudCandidates, int probeTimeoutMs);
    void markDisconnected(const std::string& reason);
    void resetPresenceStateLocked();
    void invokeOnReadyCallback();
    void invokePresenceUpdateCallback(const MmWavePresenceDecision& decision);

public:
    mmWave();
    ~mmWave();

    // Thread-safe snapshot of the latest decoded sensor reading.
    MmWaveReading getReading();

    // Thread-safe presence state derived from the rolling-window classifier.
    bool isPresent();
    MmWavePresenceState getPresenceState();
    MmWavePresenceDecision getPresenceDecision();

    void setPresenceConfig(const MmWavePresenceConfig& config);
    void setPresenceDetectionEnabled(bool enabled);

    // Register a callback invoked once the serial port opens successfully.
    void setOnReadyCallback(std::function<void()> callback);
    void setPresenceUpdateCallback(std::function<void(const MmWavePresenceDecision&)> callback);
};
