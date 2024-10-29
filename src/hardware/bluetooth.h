#ifndef BLUETOOTH_H
#define BLUETOOTH_H
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <logger.h>

/**
 * @brief An object to hold information about a Bluetooth device
 * 
 */
struct BTDevice{
    std::string mac;
    std::string name;
    std::string alias;
    std::string address;
    std::string icon;
    std::string modalias;
    std::string rssi;
    std::string txpower;
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
    std::vector<BTDevice> devices;
    std::vector<BTDevice> pairedDevices;
    std::vector<BTDevice> connectedDevices;
    std::vector<BTDevice> availableDevices;
public:
    BTControl();
    ~BTControl();
    void scanForDevices();
    bool connectToDevice(BTDevice& device);
    bool disconnectFromDevice(BTDevice& device);
    bool pairWithDevice(BTDevice& device);
    std::vector<BTDevice> getDevices();
    std::vector<BTDevice> getPairedDevices();
    std::vector<BTDevice> getConnectedDevices();
    std::vector<BTDevice> getAvailableDevices();
    
};

/**
 * @brief Communicate with mobile devices over Bluetooth
 * 
 */
class BTServices{
    std::string id_token = ""; // JWT from CubeServer API
    std::string access_token = ""; // JWT from CubeServer API
    std::string refresh_token = ""; // JWT from CubeServer API
    std::string userName = ""; // Get from user
    std::string userEmail = ""; // Derive from JWT
    std::string cubeName = ""; // Let the user set this
public:
    BTServices();
    ~BTServices();
};

/**
 * @brief Manager for all things Bluetooth
 * 
 */
class BTManager{
    BTControl* control;
    BTServices* services;
    std::jthread loopThread;
public:
    BTManager();
    ~BTManager();
};

#endif// BLUETOOTH_H
