// TODO: all of te nmcli commands can use the -t flag to get a terse version of the output. This is easier to parse and has fewer spaces. This should make it faster(?).
// TODO: refactor all the nmcli commands so that the function that use it can have a more reusable version.

#include "wifi.h"

static std::string get_nmcliField_deviceList(const std::string& field);
static std::string get_nmcliField_deviceShow(const std::string& field, const std::string& device);
static std::string get_nmcliField_deviceShow_find(const std::string& field, const std::string& searchString, const std::string& device);
static std::string get_nmcliField_deviceListActive(const std::string& field, const std::string& device);
static std::string get_nmcliField_deviceShow(const std::vector<std::string>& fields, const std::string& device);
static std::string get_nmcliField_deviceListActive(const std::vector<std::string>& fields, const std::string& device);
static std::vector<std::string> get_nmcliField_deviceListAll(const std::string& field);
static std::vector<std::string> get_nmcliField_deviceListAll(const std::vector<std::string>& fields);
static std::string stringTrim(std::string& str);
static std::string executeCommand(const std::string& command);
static std::string sanitizeInput(const std::string& input);
static std::string findDevice();
std::string replaceAll(std::string& str, const std::string& from, const std::string& to);

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
        while (true) {
            genericSleep(100); // Sleep first so that the mutex doesn't cause a deadlock
            // Do stuff
            if (WifiManager::networks.size() == 0) {
                WifiManager::getNetworks(true);
                for (auto n : getSavedNetworks()) {
                    CubeLog::debugSilly("Saved network: " + n.ssid);
                }

                for(auto n: networks){
                    CubeLog::debugSilly("Available Network: " + n.ssid + (n.connected ? " (Connected)" : ""));
                }
            }
            std::unique_lock<std::mutex> lock(mutex);
            if (!running)
                break;
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
    if (WifiManager::devName.empty() || WifiManager::devName == "") {
        devName = findDevice();
    }
    if (WifiManager::devName.empty() || WifiManager::devName == "") {
        return false;
    }
    networks.clear();
    executeCommand("nmcli device wifi rescan 2>&1");
    std::vector<std::string> fieldsToGet = { "SSID", "SIGNAL", "SECURITY", "BSSID", "FREQ", "CHAN", "RATE", "IN-USE" };
    std::vector<std::string> output = get_nmcliField_deviceListAll(fieldsToGet);
    std::map<std::string, std::string> columnToData;
    for (auto line : output) {
        if (line.empty())
            continue;
        WifiInfo network;
        std::istringstream iss(line);
        std::string field;
        for (auto c : fieldsToGet) {
            if(c == "BSSID"){
                std::string mac;
                std::getline(iss, mac, ':');
                mac += ":";
                std::getline(iss, field, ':');
                mac += field + ":";
                std::getline(iss, field, ':');
                mac += field + ":";
                std::getline(iss, field, ':');
                mac += field + ":";
                std::getline(iss, field, ':');
                mac += field + ":";
                std::getline(iss, field, ':');
                mac += field;
                replaceAll(mac, "\\:", ":");
                columnToData[c] = mac;
            }else{
                std::getline(iss, field, ':');
                columnToData[c] = stringTrim(field);
            }
        }
        network.ssid = columnToData["SSID"];
        network.signalStrength = columnToData["SIGNAL"];
        network.securityType = columnToData["SECURITY"];
        network.mac = columnToData["BSSID"];
        network.frequency = columnToData["FREQ"];
        network.channel = columnToData["CHAN"];
        network.bitrate = columnToData["RATE"];
        network.connected = columnToData["IN-USE"] == "*";

        if (network.connected) {
            std::string returnString = get_nmcliField_deviceShow("IP4.ADDRESS", devName);
            network.ip = returnString.substr(0, returnString.find("/"));
            // convert the int subnet to ip
            network.subnet = strtol(returnString.substr(returnString.find("/") + 1).c_str(), NULL, 10);
            network.gateway = get_nmcliField_deviceShow("IP4.GATEWAY", devName);
            stringTrim(network.gateway.ip);
            network.dns1 = get_nmcliField_deviceShow_find("IP4.DNS", "DNS[1]", devName);
            network.dns2 = get_nmcliField_deviceShow_find("IP4.DNS", "DNS[2]", devName);
            network.dns3 = get_nmcliField_deviceShow_find("IP4.DNS", "DNS[3]", devName);
            network.dhcp = get_nmcliField_deviceShow_find("DHCP4.OPTION", "dhcp_server_identifier", devName);
            returnString = get_nmcliField_deviceShow_find("DHCP4.OPTION", "lease_time", devName);
            network.dhcpLease = strtol(returnString.substr(returnString.find("=") + 2).c_str(), NULL, 10);
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
    std::string output = get_nmcliField_deviceListActive("IN-USE", devName);
    return output.find("*") != std::string::npos;
}

// TODO: many of the functions below use the same boilerplate code, consider refactoring.

IP_Address WifiManager::getIP()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string output = get_nmcliField_deviceShow("IP4.ADDRESS", devName);
    return IP_Address(stringTrim(output));
}

CIDR_Subnet WifiManager::getSubnet()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string output = get_nmcliField_deviceShow("IP4.ADDRESS", devName);
    output = output.substr(output.find("/") + 1);
    return CIDR_Subnet(strtol(stringTrim(output).c_str(), NULL, 10));
}

IP_Address WifiManager::getGateway()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string output = get_nmcliField_deviceShow("IP4.GATEWAY", devName);
    return IP_Address(stringTrim(output));
}

std::vector<IP_Address> WifiManager::getDNS()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return std::vector<IP_Address>();
    std::string output = get_nmcliField_deviceShow_find("IP4.DNS", "DNS[1]", devName);
    std::vector<IP_Address> dnsList;
    dnsList.push_back(IP_Address(stringTrim(output)));
    output = get_nmcliField_deviceShow_find("IP4.DNS", "DNS[2]", devName);
    if (output.length() > 1)
        dnsList.push_back(IP_Address(stringTrim(output)));
    output = get_nmcliField_deviceShow_find("IP4.DNS", "DNS[3]", devName);
    if (output.length() > 1)
        dnsList.push_back(IP_Address(stringTrim(output)));
    return dnsList;
}

std::string WifiManager::getAP_MAC()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return "";
    std::string output = get_nmcliField_deviceListActive("BSSID", devName);
    return stringTrim(output);
}

std::string WifiManager::getLocalMAC()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return "";
    std::string output = get_nmcliField_deviceShow("GENERAL.HWADDR", devName);
    return stringTrim(output);
}

std::string WifiManager::getSSID()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return "";
    std::string output = get_nmcliField_deviceListActive("SSID", devName);
    return stringTrim(output);
}

std::string WifiManager::getSignalStrength()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return "";
    std::string output = get_nmcliField_deviceListActive("SIGNAL", devName);
    return stringTrim(output);
}

std::string WifiManager::getSecurityType()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return "";
    std::string output = get_nmcliField_deviceListActive("SECURITY", devName);
    return stringTrim(output);
}

std::string WifiManager::getFrequency()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return "";
    std::string output = get_nmcliField_deviceListActive("FREQ", devName);
    return stringTrim(output);
}

std::string WifiManager::getChannel()
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return "";
    std::string output = get_nmcliField_deviceListActive("CHAN", devName);
    return stringTrim(output);
}

bool WifiManager::setProxy(const IP_Address& ip, const std::string& port)
{
    return false;
}

bool WifiManager::setDNS(const IP_Address& dns)
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string result = executeCommand("nmcli connection modify " + devName + " ipv4.dns \"" + dns.ip + "\" 2>&1");
    return result.empty();
}

bool WifiManager::setIP(const IP_Address& ip, const IP_Address& subnet, const IP_Address& gateway)
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string result = executeCommand("nmcli connection modify " + devName + " ipv4.method manual ipv4.addresses \"" + ip.ip + "/" + subnet.ip + "\" ipv4.gateway \"" + gateway.ip + "\" 2>&1");
    return result.empty();
}

bool WifiManager::setHostname(const std::string& hostname)
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string result = executeCommand("nmcli connection modify " + devName + " connection.autoconnect-priority 0 2>&1");
    return result.empty();
}

bool WifiManager::setDHCP(bool enable)
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string result = executeCommand("nmcli connection modify " + devName + " connection.autoconnect-priority 0 2>&1");
    return result.empty();
}

bool WifiManager::setDNS(const IP_Address& dns1, const IP_Address& dns2)
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
    std::string result = executeCommand("nmcli connection modify " + devName + " ipv4.dns \"" + dns1.ip + " " + dns2.ip + "\" 2>&1");
    return result.empty();
}

bool WifiManager::setVPN(const IP_Address& ip, const std::string& port, const std::string& user, const std::string& pass)
{
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        devName = findDevice();
    if (WifiManager::devName.empty() || WifiManager::devName == "")
        return false;
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
        if (line.empty())
            continue;
        if (line.find("wifi") != std::string::npos) {
            WifiInfo network;
            auto str = line.substr(0, UUID_index);
            ;
            network.ssid = stringTrim(str);
            networks.push_back(network);
        }
    }
    return networks;
}

////////////////////////////////////////////////////////////////////////////////////////

#ifdef __linux__
static std::string executeCommand(const std::string& command)
{
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
static std::string executeCommand(const std::string& command)
{
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

static std::string sanitizeInput(const std::string& input)
{
    std::string sanitized = input;

    // Remove any character that is not alphanumeric or a common safe character
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
                        [](char c) {
                            return !(std::isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.' || c == '/');
                        }),
        sanitized.end());

    return sanitized;
}

static std::string stringTrim(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(),
        str.end());
    return str;
}

static std::string findDevice()
{
    std::string output = executeCommand("nmcli device show 2>&1");
    std::istringstream iss(output);
    std::string line;
    std::string dev;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        if (line.find("GENERAL.DEVICE") != std::string::npos) {
            dev = line.substr(line.find(":") + 1);
            stringTrim(dev);
        }
        std::string devType;
        if (line.find("GENERAL.TYPE") != std::string::npos && line.find("wifi") != std::string::npos) {
            return dev;
        }
    }
    return "";
}

static std::string get_nmcliField_deviceShow(const std::string& field, const std::string& device)
{
    std::string commandReturned = executeCommand("nmcli -t -f " + field + " device show " + device + " 2>&1");
    return commandReturned.substr(commandReturned.find(":") + 1);
}

static std::string get_nmcliField_deviceShow_find(const std::string& field, const std::string& searchString, const std::string& device)
{
    std::string commandReturned = executeCommand("nmcli -t -f " + field + " device show " + device + " 2>&1");
    std::istringstream iss(commandReturned);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        if (line.find(searchString) != std::string::npos) {
            return line.substr(line.find(":") + 1);
        }
    }
    return "";
}

static std::string get_nmcliField_deviceListActive(const std::string& field, const std::string& device)
{
    std::string commandReturned = executeCommand("nmcli -t -f IN-USE," + field + " device wifi list 2>&1");

    std::istringstream iss(commandReturned);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        if (line.find("*") != std::string::npos) {
            return line.substr(line.find(":") + 1);
        }
    }
    return "";
}

static std::vector<std::string> get_nmcliField_deviceListAll(const std::string& field)
{
    std::string commandReturned = executeCommand("nmcli -t -f " + field + " device wifi list 2>&1");
    std::vector<std::string> returnVals;
    std::istringstream iss(commandReturned);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        returnVals.push_back(line);
    }
    return returnVals;
}

static std::string get_nmcliField_deviceShow(const std::vector<std::string>& fields, const std::string& device)
{
    std::string fieldsConcat = "";
    for (auto f : fields) {
        fieldsConcat += f + ",";
    }
    std::string commandReturned = executeCommand("nmcli -t -f " + fieldsConcat + " device show " + device + " 2>&1");
    return commandReturned.substr(commandReturned.find(":") + 1);
}

static std::string get_nmcliField_deviceListActive(const std::vector<std::string>& fields, const std::string& device)
{
    std::string fieldsConcat = "";
    for (auto f : fields) {
        fieldsConcat += f + ",";
    }
    std::string commandReturned = executeCommand("nmcli -t -f IN-USE," + fieldsConcat + " device wifi list 2>&1");
    std::istringstream iss(commandReturned);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        if (line.find("*") != std::string::npos) {
            return line.substr(line.find(":") + 1);
        }
    }
    return "";
}

static std::vector<std::string> get_nmcliField_deviceListAll(const std::vector<std::string>& fields)
{
    std::string fieldsConcat = "";
    for (auto f : fields) {
        fieldsConcat += f + ",";
    }
    std::string commandReturned = executeCommand("nmcli -t -f " + fieldsConcat + " device wifi list 2>&1");
    std::vector<std::string> returnVals;
    std::istringstream iss(commandReturned);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        returnVals.push_back(line);
    }
    return returnVals;
}

std::string replaceAll(std::string& str, const std::string& from, const std::string& to) { 
    size_t start_pos = 0; 
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) { 
        str.replace(start_pos, from.length(), to); 
        start_pos += to.length(); // Move past the replaced part 
    }
    return str;
}