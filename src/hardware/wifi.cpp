#include "wifi.h"

std::vector<WifiInfo> WifiManager::networks = std::vector<WifiInfo>();
std::jthread WifiManager::loopThread;
std::mutex WifiManager::mutex;
bool WifiManager::running = true;

// TODO: Implement the wifi class
WifiManager::WifiManager()
{
    loopThread = std::jthread([&]() {
        while(true){
            // Do stuff
            if(WifiManager::networks.size() == 0){
                WifiManager::getNetworks(true);
                for(auto net: networks){
                    CubeLog::info("=====================================");
                    CubeLog::info("SSID: " + net.ssid);
                    CubeLog::info("Signal Strength: " + net.signalStrength);
                    CubeLog::info("Security Type: " + net.securityType);
                    CubeLog::info(net.connected?"Connected":"Not Connected");
                    CubeLog::info("IP: " + net.ip.toString());
                    CubeLog::info("Subnet: " + net.subnet.toString());
                    CubeLog::info("Gateway: " + net.gateway.toString());
                    CubeLog::info("DNS1: " + net.dns1.toString());
                    CubeLog::info("DNS2: " + net.dns2.toString());
                    CubeLog::info("DNS3: " + net.dns3.toString());
                    CubeLog::info("MAC: " + net.mac);
                    CubeLog::info("Frequency: " + net.frequency);
                    CubeLog::info("Channel: " + net.channel);
                    CubeLog::info("Bitrate: " + net.bitrate);
                    CubeLog::info("=====================================");
                }
            }
            std::unique_lock<std::mutex> lock(mutex);
            if (!running) break;
            genericSleep(100);
        }
    });
}

WifiManager::~WifiManager()
{
    
}

/**
 * @brief Enable the wifi device
 *
 * @return true if the device is enabled after the call
 */
bool WifiManager::enable()
{
    std::string result = executeCommand("nmcli radio wifi on 2>&1");
    return result.empty();
}

/**
 * @brief Disable the wifi device
 *
 * @return true if the device is disabled after the call
 */
bool WifiManager::disable()
{
    std::string result = executeCommand("nmcli radio wifi off 2>&1");
    return result.empty();
}

bool WifiManager::scan()
{
    std::string output = executeCommand("nmcli -f SSID,SIGNAL,SECURITY device wifi list 2>&1");
    std::istringstream iss(output);
    std::string line;

    // Skip the header line
    std::getline(iss, line);

    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Split the line into fields
        std::string ssid = line.substr(0, 32);
        std::string signal = line.substr(33, 7);
        std::string security = line.substr(41);

        // Trim whitespace
        ssid.erase(ssid.find_last_not_of(" \t") + 1);
        signal.erase(0, signal.find_first_not_of(" \t"));
        signal.erase(signal.find_last_not_of(" \t") + 1);
        security.erase(0, security.find_first_not_of(" \t"));
        security.erase(security.find_last_not_of(" \t") + 1);

        WifiInfo network;
        network.ssid = ssid;
        network.signalStrength = signal;
        network.securityType = security;

        networks.push_back(network);
    }
    return true;
}

std::vector<std::string> WifiManager::getNetworks(bool refresh)
{
    if (refresh) {
        networks.clear();
        scan();
    }
    std::vector<std::string> networkList;
    for (auto& network : networks) {
        networkList.push_back(network.ssid);
    }
    return networkList;
}

bool WifiManager::connect(const std::string& network, const std::string& password)
{
    return false;
}

bool WifiManager::forgetNetwork(const std::string& network)
{
    return false;
}

bool WifiManager::disconnect()
{
    return false;
}

bool WifiManager::isConnected()
{
    return false;
}

IP_Address WifiManager::getIP()
{
    return 0;
}

IP_Address WifiManager::getSubnet()
{
    return 0;
}

IP_Address WifiManager::getGateway()
{
    return 0;
}

std::vector<IP_Address> WifiManager::getDNS()
{
    return std::vector<IP_Address>();
}

std::string WifiManager::getMAC()
{
    return "";
}

std::string WifiManager::getSSID()
{
    return "";
}

std::string WifiManager::getSignalStrength()
{
    return "";
}

std::string WifiManager::getSecurityType()
{
    return "";
}

std::string WifiManager::getFrequency()
{
    return "";
}

std::string WifiManager::getChannel()
{
    return "";
}

bool WifiManager::setProxy(const IP_Address& ip, const std::string& port)
{
    return false;
}

bool WifiManager::setDNS(const IP_Address& dns)
{
    return false;
}

bool WifiManager::setIP(const IP_Address& ip, const IP_Address& subnet, const IP_Address& gateway)
{
    return false;
}

bool WifiManager::setHostname(const std::string& hostname)
{
    return false;
}

bool WifiManager::setDHCP(bool enable)
{
    return false;
}

bool WifiManager::setDNS(const IP_Address& dns1, const IP_Address& dns2)
{
    return false;
}

bool WifiManager::setVPN(const IP_Address& ip, const std::string& port, const std::string& user, const std::string& pass)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////

static std::string executeCommand(const std::string& command) {
    std::array<char, 256> buffer;
    std::string result;

    // Open pipe to file
    std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "popen() failed!" << std::endl;
        return "";
    }

    // Read till end of process
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }

    return result;
}

static std::string sanitizeInput(const std::string& input) {
    std::string sanitized = input;

    // Remove any character that is not alphanumeric or a common safe character
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](char c) {
            return !(std::isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.' || c == '/');
        }), sanitized.end());

    return sanitized;
}