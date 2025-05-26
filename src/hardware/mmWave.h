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


#pragma once
#include <memory>
#include <string>
#include <vector>
#ifndef LOGGER_H
#include <logger.h>
#endif
#ifdef ___linux__
#include <iostream>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#endif

// TODO: add some ifdefs and defines for the port name on RasPi
// TODO: add ifdefs for Windows so that this will compile on Windows
// For windows, we should just mock the mmWave sensor.

#define COMMAND_HEADER { 0xFD, 0xFC, 0xFB, 0xFA }
#define COMMAND_TAIL { 0x04, 0x03, 0x02, 0x01 }
#define REPORT_HEADER { 0xF4, 0xF3, 0xF2, 0xF1 }
#define REPORT_TAIL { 0xF8, 0xF7, 0xF6, 0xF5 }

#define ENABLE_CONFIG_MODE { 0xff, 0x00, 0x01, 0x00 }
#define ENABLE_CONFIG_MODE_ACK { 0xff, 0x01, 0x00, 0x00, 0x01, 0x00, 0x40, 0x00 }
#define DISABLE_CONFIG_MODE { 0xFE, 0x00 }
#define DISABLE_CONFIG_MODE_ACK { 0xFE, 0x01, 0x00, 0x00 }

#define ENABLE_ENGINEERING_MODE { 0x62, 0x00 }
#define ENABLE_ENGINEERING_MODE_ACK { 0x62, 0x01, 0x00, 0x00 }
#define DISABLE_ENGINEERING_MODE { 0x63, 0x00 }
#define DISABLE_ENGINEERING_MODE_ACK { 0x63, 0x01, 0x00, 0x00 }

class Response {
public:
    bool success = false;
    std::vector<uint8_t> data = {};
    std::string hexStr = "";

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

class mmWave {
    uint8_t targetState = 0;
    uint16_t movingTargetDistance = 0;
    uint8_t movingTargetEnergy = 0;
    uint16_t stationaryTargetDistance = 0;
    uint8_t stationaryTargetEnergy = 0;
    uint16_t detectionDistance = 0;

    int serialPort_h;
    bool configModeEnabled = false;
    std::unique_ptr<std::jthread> readerThread;

    Response sendCommand(std::vector<uint8_t> command);
    Response sendCommand(std::vector<uint8_t> command, std::vector<uint8_t> ack);
    bool enableConfigMode();
    bool disableConfigMode();
    bool enableEngineeringMode();
    bool disableEngineeringMode();
    bool setMaxMovingDistanceGate(int distance);
    bool setMaxRestingDistanceGate(int distance);
    bool setUnmannedDuration(int duration);
    bool setDistanceGateSensitivity(int sensitivity);
    bool setMotionSensitivity(int sensitivity);
    bool setRestingSensitivity(int sensitivity);
    void restartModule();
    Response readDataFrame();
    void decodeDataFrame(Response response);

public:
    mmWave();
    ~mmWave();
};
