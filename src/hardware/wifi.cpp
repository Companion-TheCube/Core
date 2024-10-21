#include "wifi.h"

static void stringTrim(std::string& str);

std::vector<WifiInfo> WifiManager::networks = std::vector<WifiInfo>();
std::jthread WifiManager::loopThread;
std::mutex WifiManager::mutex;
bool WifiManager::running = true;
WifiInfo WifiManager::currentNetwork = WifiInfo();

// TODO: finish implementing the WifiManager class
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
                    CubeLog::info("DHCP: " + net.dhcp.toString());
                    CubeLog::info("DHCP Lease: " + std::to_string(net.dhcpLease));
                    CubeLog::info("Hostname: " + net.hostname);
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
    std::unique_lock<std::mutex> lock(mutex);
    running = false;
    lock.unlock();
    loopThread.join();
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
    networks.clear();
    executeCommand("nmcli device wifi rescan 2>&1");
    genericSleep(500);
    std::string output = executeCommand("nmcli -f SSID,SIGNAL,SECURITY,BSSID,FREQ,CHAN,RATE,IN-USE device wifi list 2>&1");
    std::istringstream iss(output);
    std::string line;
    uint16_t letterIndex = 0;
    std::vector<size_t> columnIndexes;
    std::vector<std::string> columns;

    // we read the first line to get the headers
    std::getline(iss, line);
    // read this line to get the columns
    bool columnFound = false;
    while(letterIndex < line.size()){
        if(line[letterIndex] == ' ') {
            letterIndex++;
            columnFound = false;
            continue;
        }else{
            columnFound = true;
        }
        if(columnFound){
            columnIndexes.push_back(letterIndex);
            columns.push_back(line.substr(letterIndex, line.find(' ', letterIndex) - letterIndex));
            letterIndex = line.find(' ', letterIndex);
        }
    }

    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        WifiInfo network;
        for(size_t i = 0; i < columns.size(); i++){
            size_t endIndex = i+1 < columnIndexes.size() ? columnIndexes[i+1] : line.size();
            if(columns[i] == "SSID"){
                network.ssid = line.substr(columnIndexes[i], endIndex - columnIndexes[i]);
                stringTrim(network.ssid);
            }else if(columns[i] == "SIGNAL"){
                network.signalStrength = line.substr(columnIndexes[i], endIndex - columnIndexes[i]);
                stringTrim(network.signalStrength);
            }else if(columns[i] == "SECURITY"){
                network.securityType = line.substr(columnIndexes[i], endIndex - columnIndexes[i]);
                stringTrim(network.securityType);
            }else if(columns[i] == "BSSID"){
                network.mac = line.substr(columnIndexes[i], endIndex - columnIndexes[i]);
                stringTrim(network.mac);
            }else if(columns[i] == "FREQ"){
                network.frequency = line.substr(columnIndexes[i], endIndex - columnIndexes[i]);
                stringTrim(network.frequency);
            }else if(columns[i] == "CHAN"){
                network.channel = line.substr(columnIndexes[i], endIndex - columnIndexes[i]);
                stringTrim(network.channel);
            }else if(columns[i] == "RATE"){
                network.bitrate = line.substr(columnIndexes[i], endIndex - columnIndexes[i]);
                stringTrim(network.bitrate);
            }else if(columns[i] == "IN-USE"){
                network.connected = line.find("*", columnIndexes[i]) != std::string::npos;
            }
        }
        if(network.connected){
            std::string ipOutput = executeCommand("nmcli -f IP4,DHCP4 device show wlan0 2>&1");
            std::istringstream ipIss(ipOutput);
            std::string ipLine;
            while (std::getline(ipIss, ipLine)) {
                if (ipLine.empty()) continue;
                if(ipLine.find("IP4.ADDRESS") != std::string::npos){
                    network.ip = IP_Address(ipLine.substr(ipLine.find(":") + 1));
                    stringTrim(network.ip.ip);
                }else if(ipLine.find("DHCP4") != std::string::npos && ipLine.find("dhcp_server_identifier") != std::string::npos){
                    network.dhcp = IP_Address(ipLine.substr(ipLine.find("=") + 1));
                    stringTrim(network.dhcp.ip);
                }else if(ipLine.find("IP4.GATEWAY") != std::string::npos){
                    network.gateway = IP_Address(ipLine.substr(ipLine.find(":") + 1));
                    stringTrim(network.gateway.ip);
                }else if(ipLine.find("IP4.DNS") != std::string::npos){
                    if(network.dns1.ip.empty()){
                        network.dns1 = IP_Address(ipLine.substr(ipLine.find(":") + 1));
                        stringTrim(network.dns1.ip);
                    }else if(network.dns2.ip.empty()){
                        network.dns2 = IP_Address(ipLine.substr(ipLine.find(":") + 1));
                        stringTrim(network.dns2.ip);
                    }else if(network.dns3.ip.empty()){
                        network.dns3 = IP_Address(ipLine.substr(ipLine.find(":") + 1));
                        stringTrim(network.dns3.ip);
                    }
                }else if(ipLine.find("DHCP4.OPTION") != std::string::npos && ipLine.find("lease_time") != std::string::npos){
                    network.dhcpLease = std::stoul(ipLine.substr(ipLine.find("=") + 1));
                }else if(ipLine.find("DHCP4.OPTION") != std::string::npos && ipLine.find("hostname") != std::string::npos){
                    network.hostname = ipLine.substr(ipLine.find("=") + 1);
                    stringTrim(network.hostname);
                }else if(ipLine.find("DHCP4.OPTION") != std::string::npos && ipLine.find("subnet_mask") != std::string::npos){
                    network.subnet = IP_Address(ipLine.substr(ipLine.find("=") + 1));
                    stringTrim(network.subnet.ip);
                }
            }
        }
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
    sanitizeInput(network);
    sanitizeInput(password);
    std::string result = executeCommand("nmcli device wifi connect \"" + network + "\" password \"" + password + "\" 2>&1");
    return result.empty();
}

bool WifiManager::forgetNetwork(const std::string& network)
{
    sanitizeInput(network);
    std::string result = executeCommand("nmcli connection delete \"" + network + "\" 2>&1");
    return result.empty();
}

bool WifiManager::disconnect()
{
    std::string result = executeCommand("nmcli device disconnect wlan0 2>&1");
    return result.empty();
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

#ifdef __linux__
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
#endif
#ifdef _WIN32
static std::string executeCommand(const std::string& command) {
    std::array<char, 256> buffer;
    std::string result;

    // Open pipe to file
    std::shared_ptr<FILE> pipe(_popen(command.c_str(), "r"), _pclose);
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
#endif

static std::string sanitizeInput(const std::string& input) {
    std::string sanitized = input;

    // Remove any character that is not alphanumeric or a common safe character
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](char c) {
            return !(std::isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.' || c == '/');
        }), sanitized.end());

    return sanitized;
}

static void stringTrim(std::string& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), str.end());
}

