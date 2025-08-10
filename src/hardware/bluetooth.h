/*
██████╗ ██╗     ██╗   ██╗███████╗████████╗ ██████╗  ██████╗ ████████╗██╗  ██╗   ██╗  ██╗
██╔══██╗██║     ██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔═══██╗╚══██╔══╝██║  ██║   ██║  ██║
██████╔╝██║     ██║   ██║█████╗     ██║   ██║   ██║██║   ██║   ██║   ███████║   ███████║
██╔══██╗██║     ██║   ██║██╔══╝     ██║   ██║   ██║██║   ██║   ██║   ██╔══██║   ██╔══██║
██████╔╝███████╗╚██████╔╝███████╗   ██║   ╚██████╔╝╚██████╔╝   ██║   ██║  ██║██╗██║  ██║
╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝  ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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
#ifndef BLUETOOTH_H
#define BLUETOOTH_H
#include <array>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#ifndef HTTPLIB_INCLUDED
#define HTTPLIB_INCLUDED
#include <httplib.h>
#endif
#ifdef __linux__
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#ifdef PRODUCTION_BUILD
#ifdef __linux__
#define BT_MANAGER_ADDRESS "/tmp/cube/bt_manager.sock"
#else
#define BT_MANAGER_ADDRESS "http://localhost:55290"
#endif
#else
#define BT_MANAGER_ADDRESS "http://localhost:55290"
#endif
#ifdef __linux__
#define BT_MANAGER_EXECUTABLE "bt_manager"
#else
#define BT_MANAGER_EXECUTABLE "bt_manager.exe"
#endif
#include "../api/api.h"
#include "nlohmann/json.hpp"
#include "utils.h"
#include "uuid.h"

/**
 * @brief An object to hold information about a Bluetooth device
 *
 */
struct BTDevice {
    std::string mac;
    std::string name;
    std::string alias;
    std::string rssi;
    std::string manufacturer;
    bool paired;
    bool connected;
    bool trusted;
    bool blocked;
};

/**
 * @brief Control the Bluetooth hardware (turn on/off, scan for devices, connect to devices)
 *
 */
class BTControl {
    std::vector<BTDevice> devices; // maintains a list of all devices, connected, pair or otherwise
    std::vector<BTDevice> pairedDevices; // maintains a list of paired devices
    std::vector<BTDevice> connectedDevices; // maintains a list of connected devices
    std::vector<BTDevice> availableDevices; // maintains a list of available devices (paired and connected)
    httplib::Server* server;
    httplib::Client* client;
    std::string client_id;
    std::string address;
    int port;
    std::jthread* serverThread;
    std::jthread* heartbeatThread;
    std::mutex m;
    std::condition_variable cv;
    uuids::uuid authUUID;
#ifndef PRODUCTION_BUILD
    std::jthread* mockThread;
#endif

public:
    BTControl(uuids::uuid authUUID);
    ~BTControl();
    bool scanForDevices();
    bool stopScanning();
    bool makeVisible(bool visible);
    bool connectToDevice(BTDevice& device);
    bool disconnectFromDevice(BTDevice& device);
    bool pairWithDevice(BTDevice& device);
    std::vector<BTDevice> getDevices();
    std::vector<BTDevice> getPairedDevices();
    std::vector<BTDevice> getConnectedDevices();
    std::vector<BTDevice> getAvailableDevices();
};

struct BTCharacteristic {
    std::string name;
    std::function<void(std::string)> callback_void_string;
    std::function<std::string()> callback_string_void;
    std::function<void(std::string, std::string)> callback_void_string_string;
    std::function<std::string(std::string)> callback_string_string;
    std::function<std::string(std::vector<std::string>)> callback_string_vector_string;
    std::function<std::vector<std::string>(std::string)> callback_vector_string_string;
    std::function<std::vector<std::string>(std::vector<std::string>)> callback_vector_string_vector_string;
    uuids::uuid uuid;
    enum class CBType {
        VOID_STRING,
        STRING_VOID,
        VOID_STRING_STRING,
        STRING_STRING,
        STRING_VECTOR_STRING,
        VECTOR_STRING_STRING,
        VECTOR_STRING_VECTOR_STRING
    } cbType;
};

/**
 * @brief Communicate with mobile devices over Bluetooth
 *
 */
class BTService {
    httplib::Client* client;
    httplib::Server* server;
    std::vector<BTCharacteristic*> characteristics;
    bool characteristicsLocked = false;
    std::string client_id = "";
    nlohmann::json config;
    uuids::uuid authUUID;

public:
    BTService(const nlohmann::json& config, httplib::Server* server, uuids::uuid authUUID);
    ~BTService();
    void addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<void(std::string)> callback);
    void addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<std::string()> callback);
    void addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<void(std::string, std::string)> callback);
    void addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<std::string(std::string)> callback);
    void addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<std::string(std::vector<std::string>)> callback);
    void addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<std::vector<std::string>(std::string)> callback);
    void addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<std::vector<std::string>(std::vector<std::string>)> callback);
    bool start();
    std::string getClientId();
};

/**
 * @brief Manager for all things Bluetooth
 *
 */
class BTManager : public AutoRegisterAPI<BTManager> {
    httplib::Client* client;
    httplib::Server* server;
    std::jthread* serverThread;
    std::jthread* heartbeatThread;
    BTControl* control;
    std::vector<BTService*> services;
    std::string client_id;
    nlohmann::json config;
    uuids::uuid authUUID;

public:
    BTManager();
    ~BTManager();
    void addService(BTService* service);
    void removeService(BTService* service);
    // CUBE API Interface
    HttpEndPointData_t getHttpEndpointData();
    constexpr std::string getInterfaceName() const override;
};

#endif // BLUETOOTH_H
