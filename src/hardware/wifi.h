#ifndef WIFI_H
#define WIFI_H
#include <utils.h>
#include "globalSettings.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <list>


struct IP_Address {
    IP_Address(){
        this->ip = "";
    };
    IP_Address(std::string ip)
        : ip(ip) {};
    IP_Address(uint32_t ip)
    {
        // convert the 32 bit integer to an ip address
        uint8_t ip1 = (ip >> 24) & 0xFF;
        uint8_t ip2 = (ip >> 16) & 0xFF;
        uint8_t ip3 = (ip >> 8) & 0xFF;
        uint8_t ip4 = ip & 0xFF;
        this->ip = std::to_string(ip1) + "." + std::to_string(ip2) + "." + std::to_string(ip3) + "." + std::to_string(ip4);
    };
    std::string ip;
    uint32_t IP_uint32()
    {
        // convert the ip address to a 32 bit integer
        uint8_t ip1, ip2, ip3, ip4;
        sscanf(this->ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &ip1, &ip2, &ip3, &ip4);
        return (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;
    };
    std::string toString()
    {
        return this->ip;
    };
};

struct WifiInfo {
    WifiInfo(){}; // default constructor
    WifiInfo(std::string ssid, std::string password)
        : ssid(ssid)
        , password(password)
        , connected(false){};
    std::string ssid = "";
    std::string password = "";
    std::string mac = "";
    IP_Address ip;
    IP_Address subnet;
    IP_Address gateway;
    IP_Address dns1;
    IP_Address dns2;
    IP_Address dns3;
    std::string signalStrength = "";
    std::string securityType = "";
    std::string frequency = "";
    std::string channel = "";
    std::string bitrate = "";
    bool connected = false;
};

class WifiManager {
public:
    WifiManager();
    ~WifiManager();
    static bool enable();
    static bool disable();
    static bool scan();
    static std::vector<std::string> getNetworks(bool refresh = false);
    static bool connect(const std::string& network, const std::string& password);
    static bool forgetNetwork(const std::string& network);
    static bool disconnect();
    static bool isConnected();
    static IP_Address getIP();
    static IP_Address getSubnet();
    static IP_Address getGateway();
    static std::vector<IP_Address> getDNS();
    static std::string getMAC();
    static std::string getSSID();
    static std::string getSignalStrength();
    static std::string getSecurityType();
    static std::string getFrequency();
    static std::string getChannel();
    static bool setProxy(const IP_Address& ip, const std::string& port);
    static bool setDNS(const IP_Address& dns);
    static bool setIP(const IP_Address& ip, const IP_Address& subnet, const IP_Address& gateway);
    static bool setHostname(const std::string& hostname);
    static bool setDHCP(bool enable);
    static bool setDNS(const IP_Address& dns1, const IP_Address& dns2);
    static bool setVPN(const IP_Address& ip, const std::string& port, const std::string& user, const std::string& pass);

private:
    static std::vector<WifiInfo> networks;
    static std::jthread loopThread;
    static std::mutex mutex;
    static bool running;

};

static std::string executeCommand(const std::string& command);
static std::string sanitizeInput(const std::string& input);

#endif // WIFI_H
