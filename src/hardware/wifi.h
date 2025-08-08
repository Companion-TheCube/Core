/*
██╗    ██╗██╗███████╗██╗   ██╗  ██╗
██║    ██║██║██╔════╝██║   ██║  ██║
██║ █╗ ██║██║█████╗  ██║   ███████║
██║███╗██║██║██╔══╝  ██║   ██╔══██║
╚███╔███╔╝██║██║     ██║██╗██║  ██║
 ╚══╝╚══╝ ╚═╝╚═╝     ╚═╝╚═╝╚═╝  ╚═╝
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

#ifndef WIFI_H
#define WIFI_H
#include "globalSettings.h"
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <utils.h>
#include <vector>

struct CIDR_Subnet {
    CIDR_Subnet()
    {
        this->mask = "";
        this->cidr = 0;
    };
    CIDR_Subnet(const std::string& mask, uint8_t cidr)
        : mask(mask)
        , cidr(cidr) {};
    CIDR_Subnet(const std::string& mask)
    {
        this->mask = mask;
        this->cidr = 0;
    };
    CIDR_Subnet(uint8_t cidr)
    {
        this->cidr = cidr;
        // convert cidr to string mask
        uint32_t mask = 0xFFFFFFFF << (32 - cidr);
        uint8_t mask1 = (mask >> 24) & 0xFF;
        uint8_t mask2 = (mask >> 16) & 0xFF;
        uint8_t mask3 = (mask >> 8) & 0xFF;
        uint8_t mask4 = mask & 0xFF;
        this->mask = std::to_string(mask1) + "." + std::to_string(mask2) + "." + std::to_string(mask3) + "." + std::to_string(mask4);
    };
    std::string mask;
    uint8_t cidr;
    std::string toString()
    {
        return this->mask;
    };

    uint32_t CIDR_uint32()
    {
        uint32_t mask = 0xFFFFFFFF << (32 - this->cidr);
        return mask;
    };

    uint32_t CIDR_uint32(uint8_t cidr)
    {
        uint32_t mask = 0xFFFFFFFF << (32 - cidr);
        return mask;
    };
};

struct IP_Address {
    IP_Address()
    {
        this->ip = "";
    };
    IP_Address(const std::string& ip)
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
    WifiInfo() {}; // default constructor
    WifiInfo(const std::string& ssid, const std::string& password)
        : ssid(ssid)
        , password(password)
        , connected(false) {};
    std::string ssid = "";
    std::string password = "";
    std::string mac = "";
    IP_Address ip;
    CIDR_Subnet subnet;
    IP_Address gateway;
    IP_Address dns1;
    IP_Address dns2;
    IP_Address dns3;
    IP_Address dhcp;
    unsigned long dhcpLease = 0;
    std::string hostname = "";
    std::string signalStrength = "";
    std::string securityType = "";
    std::string frequency = "";
    std::string channel = "";
    std::string bitrate = "";
    bool connected = false;
    std::string to_string()
    {
        std::string output = "";
        output += "SSID: " + ssid + "\n";
        output += "Password: " + password + "\n";
        output += "MAC: " + mac + "\n";
        output += "IP: " + ip.toString() + "\n";
        output += "Subnet: " + subnet.toString() + "\n";
        output += "Gateway: " + gateway.toString() + "\n";
        output += "DNS1: " + dns1.toString() + "\n";
        output += "DNS2: " + dns2.toString() + "\n";
        output += "DNS3: " + dns3.toString() + "\n";
        output += "DHCP: " + dhcp.toString() + "\n";
        output += "DHCP Lease: " + std::to_string(dhcpLease) + "\n";
        output += "Hostname: " + hostname + "\n";
        output += "Signal Strength: " + signalStrength + "\n";
        output += "Security Type: " + securityType + "\n";
        output += "Frequency: " + frequency + "\n";
        output += "Channel: " + channel + "\n";
        output += "Bitrate: " + bitrate + "\n";
        output += "Connected: " + std::to_string(connected) + "\n";
        return output;
    };
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
    static CIDR_Subnet getSubnet();
    static IP_Address getGateway();
    static std::vector<IP_Address> getDNS();
    static std::string getAP_MAC();
    static std::string getLocalMAC();
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
    static std::vector<WifiInfo> getSavedNetworks();
    static std::mutex commandMutex;

private:
    static std::vector<WifiInfo> networks;
    static std::jthread loopThread;
    static std::mutex mutex;
    static bool running;
    static WifiInfo currentNetwork;
    static std::string devName;
};

#endif // WIFI_H
