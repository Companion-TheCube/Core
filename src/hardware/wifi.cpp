// TODO: all of te nmcli commands can use the -t flag to get a terse version of the output. This is easier to parse and has fewer spaces. This should make it faster(?).
// TODO: refactor all the nmcli commands so that the function that use it can have a more reusable version.

#include "wifi.h"

static std::string stringTrim(std::string& str);
static std::string executeCommand(const std::string& command);
static std::string sanitizeInput(const std::string& input);
static std::string findDevice();

std::vector<WifiInfo> WifiManager::networks = std::vector<WifiInfo>();
std::jthread WifiManager::loopThread;
std::mutex WifiManager::mutex;
bool WifiManager::running = true;
WifiInfo WifiManager::currentNetwork = WifiInfo();
std::string WifiManager::devName = "";
std::mutex WifiManager::commandMutex;

// TODO: finish implementing the WifiManager class
WifiManager::WifiManager()
{
    devName = findDevice();
    loopThread = std::jthread([&]() {
        while(true){
            // Do stuff
            if(WifiManager::networks.size() == 0){
                WifiManager::getNetworks(true);
                for(auto n : getSavedNetworks()){
                    CubeLog::debugSilly("Saved network: " + n.ssid);
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
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
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
            std::string ipOutput = executeCommand("nmcli -f IP4,DHCP4 device show " + devName + " 2>&1");
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
    std::string output = executeCommand("nmcli -f SSID,IN-USE device wifi list 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("*") != std::string::npos) return true;
    }
    return false;
}

// TODO: many of the functions below use the same boilerplate code, consider refactoring.

IP_Address WifiManager::getIP()
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string output = executeCommand("nmcli -f IP4 device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("IP4.ADDRESS") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            str = str.substr(0, str.find("/"));
            return IP_Address(stringTrim(str));
        }
    }
    return 0;
}

IP_Address WifiManager::getSubnet()
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string output = executeCommand("nmcli -f IP4 device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("IP4.ADDRESS") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            str = str.substr(str.find("/") + 1);
            return IP_Address(stringTrim(str));
        }
    }
    return 0;
}

IP_Address WifiManager::getGateway()
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string output = executeCommand("nmcli -f IP4.GATEWAY device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("IP4.GATEWAY") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            return IP_Address(stringTrim(str));
        }
    }
    return 0;
}

std::vector<IP_Address> WifiManager::getDNS()
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return std::vector<IP_Address>();
    }
    std::string output = executeCommand("nmcli -f IP4.DNS device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    std::vector<IP_Address> dnsList;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("IP4.DNS") != std::string::npos) {
            dnsList.push_back(IP_Address(line.substr(line.find(":") + 1)));
            stringTrim(dnsList.back().ip);
        }
    }
    return dnsList;
}

std::string WifiManager::getAP_MAC() // TODO: not working
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return "";
    }
    std::string output = executeCommand("nmcli -f BSSID device wifi list 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("BSSID") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            return stringTrim(str);
        }
    }
    return "";
}

std::string WifiManager::getLocalMAC() // TODO: not working
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return "";
    }
    std::string output = executeCommand("nmcli -f GENERAL device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("DEVICE") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            return stringTrim(str);
        }
    }
    return "";
}

std::string WifiManager::getSSID() // TODO: not working
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return "";
    }
    std::string output = executeCommand("nmcli -f AP device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("SSID") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            return stringTrim(str);
        }
    }
    return "";
}

std::string WifiManager::getSignalStrength()
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return "";
    }
    std::string output = executeCommand("nmcli -f AP device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("SIGNAL") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            return stringTrim(str);
        }
    }
    return "";
}

std::string WifiManager::getSecurityType()
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return "";
    }
    std::string output = executeCommand("nmcli -f AP device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("SECURITY") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            return stringTrim(str);
        }
    }
    return "";
}

std::string WifiManager::getFrequency() // TODO: not working
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return "";
    }
    std::string output = executeCommand("nmcli -f FREQ device wifi list 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("*") != std::string::npos) {
            auto str = line.substr(line.find(" ") + 1);
            return stringTrim(str);
        }
    }
    return "";
}

std::string WifiManager::getChannel()
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return "";
    }
    std::string output = executeCommand("nmcli -f AP device show " + devName + " 2>&1");
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("CHAN") != std::string::npos) {
            auto str = line.substr(line.find(":") + 1);
            return stringTrim(str);
        }
    }
    return "";
}

bool WifiManager::setProxy(const IP_Address& ip, const std::string& port)
{

    return false;
}

bool WifiManager::setDNS(const IP_Address& dns)
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string result = executeCommand("nmcli connection modify " + devName + " ipv4.dns \"" + dns.ip + "\" 2>&1");
    return result.empty();
}

bool WifiManager::setIP(const IP_Address& ip, const IP_Address& subnet, const IP_Address& gateway)
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string result = executeCommand("nmcli connection modify " + devName + " ipv4.method manual ipv4.addresses \"" + ip.ip + "/" + subnet.ip + "\" ipv4.gateway \"" + gateway.ip + "\" 2>&1");
    return result.empty();
}

bool WifiManager::setHostname(const std::string& hostname)
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string result = executeCommand("nmcli connection modify " + devName + " connection.autoconnect-priority 0 2>&1");
    return result.empty();
}

bool WifiManager::setDHCP(bool enable)
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string result = executeCommand("nmcli connection modify " + devName + " connection.autoconnect-priority 0 2>&1");
    return result.empty();
}

bool WifiManager::setDNS(const IP_Address& dns1, const IP_Address& dns2)
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string result = executeCommand("nmcli connection modify " + devName + " ipv4.dns \"" + dns1.ip + " " + dns2.ip + "\" 2>&1");
    return result.empty();
}

bool WifiManager::setVPN(const IP_Address& ip, const std::string& port, const std::string& user, const std::string& pass)
{
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        devName = findDevice();
    }
    if(WifiManager::devName.empty() || WifiManager::devName == ""){
        return false;
    }
    std::string result = executeCommand("nmcli connection modify " + devName + " vpn.data \"gateway=" + ip.ip + " port=" + port + " user=" + user + " password=" + pass + "\" 2>&1");
    return result.empty();
}

std::vector<WifiInfo> WifiManager::getSavedNetworks()
{
    std::vector<WifiInfo> networks;
    std::string output = executeCommand("nmcli c 2>&1");
    std::istringstream iss(output);
    std::string line;
    std::getline(iss, line);
    int UUID_index = line.find("UUID");
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("wifi") != std::string::npos) {
            WifiInfo network;
            auto str = line.substr(0, UUID_index);;
            network.ssid = stringTrim(str);
            networks.push_back(network);
        }
    }
    return networks;
}

////////////////////////////////////////////////////////////////////////////////////////

#ifdef __linux__
static std::string executeCommand(const std::string& command) {
    std::unique_lock<std::mutex> lock(WifiManager::commandMutex); // Prevent concurrent execution
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

static std::string stringTrim(std::string& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), str.end());
    return str;
}

static std::string findDevice(){
    std::string output = executeCommand("nmcli device show 2>&1");
    std::istringstream iss(output);
    std::string line;
    std::string dev;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.find("GENERAL.DEVICE") != std::string::npos) {
            dev = line.substr(line.find(":") + 1);
            stringTrim(dev);
        }
        std::string devType;
        if(line.find("GENERAL.TYPE") != std::string::npos && line.find("wifi") != std::string::npos){
            return dev;
        }
    }
    return "";
}
