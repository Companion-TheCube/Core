#ifndef BLUETOOTH_H
#define BLUETOOTH_H
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <logger.h>
#ifndef HTTPLIB_INCLUDED
#define HTTPLIB_INCLUDED
#include <httplib.h>
#endif
#ifdef __linux__
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
#include "utils.h"
#include <filesystem>
#ifdef __linux__
#define BT_MANAGER_ADDRESS "/tmp/cube/bt_manager.sock"
#else
#define BT_MANAGER_ADDRESS "http://localhost:55290"
#endif
#include "../api/api_i.h"

/**
 * @brief An object to hold information about a Bluetooth device
 * 
 */
struct BTDevice{
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
class BTControl{
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
public:
    BTControl();
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

struct BTCharacteristic{
    std::string name;
    std::function<void(std::string)> callback;
    std::function<std::string()> getCallback;
    std::function<void(std::string, std::string)> setCallback;
    std::function<std::string(std::string)> setGetCallback;

};

/**
 * @brief Communicate with mobile devices over Bluetooth
 * 
 */
class BTService{
    static int _port;
    httplib::Server* server;
    httplib::Client* client;
    std::string address;
    int port;
    std::jthread* serverThread;
    std::jthread* heartbeatThread;
    std::vector<BTCharacteristic> characteristics;
    bool characteristicsLocked = false;
    std::string serviceName;
    std::string client_id = "";
public:
    BTService(std::string serviceName);
    ~BTService();
    void start();
    void addCharacteristic(std::string name, std::function<void(std::string)> callback);
    void addCharacteristic(std::string name, std::function<std::string()> callback);
    void addCharacteristic(std::string name, std::function<void(std::string, std::string)> callback);
    void addCharacteristic(std::string name, std::function<std::string(std::string)> callback);

};

/**
 * @brief Manager for all things Bluetooth
 * 
 */
class BTManager: public I_API_Interface{
    BTControl* control;
    std::vector<BTService*> services;
    std::mutex mut;
    std::string id_token = ""; // JWT from CubeServer API
    std::string access_token = ""; // JWT from CubeServer API
    std::string refresh_token = ""; // JWT from CubeServer API
    std::string userName = ""; // Get from user
    std::string userEmail = ""; // Derive from JWT
    std::string cubeName = ""; // Let the user set this
public:
    BTManager();
    ~BTManager();
    std::string getUserName();
    std::string getUserEmail();
    std::string getCubeName();
    void setCubeName(std::string name);
    std::string getIdToken();
    std::string getAccessToken();
    std::string getRefreshToken();
    // CUBE API Interface
    HttpEndPointData_t getHttpEndpointData();
    std::string getInterfaceName() const;
};

#endif// BLUETOOTH_H
