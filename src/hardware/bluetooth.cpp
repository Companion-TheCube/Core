#include "bluetooth.h"

// TODO: Implement the Bluetooth functions

/**
 * This file will need to have a class for controller the hardware such as turning BT on and off,
 * scanning for devices, and connecting to devices.
 * There will also need to be a class for communicating with mobiles devices over BT.
 * Perhaps make a class to communicate with other TheCubes over BT which can allow auto configuring
 * of the network settings on a new device.
 *
 */

/**
 *
 *  TODO: In order to comply with Qt licensing, we will have to have all the code that interfaces
 * with the Qt bluetooth module in a separate application. This app will then use http over unix sockets
 * to communicate with the main application.
 */

BTControl::BTControl()
{
    // TODO: implement an http server that runs on a unix socket (or tcp for windows) that provides
    // endpoints for the BTManager app to use as callbacks for incoming events.
    // This class will have to call the setup function (or whatever i end up calling it) in the BTManager
    // to configure the callbacks.
    this->server = new httplib::Server();
#ifdef _WIN32
    this->address = "0.0.0.0";
    this->port = 55285;
#else
    unlink("/tmp/cube/bt_control.sock");
    this->address = "/tmp/cube/bt_control.sock";
    this->port = 0;
#endif

    nlohmann::json config;
    std::filesystem::path configPath = std::filesystem::current_path() / "data" / "bt_control.json";
    std::ifstream file(configPath);
    if (file.good()) {
        file >> config;
    } else {
        CubeLog::error("Error opening file: " + configPath.string());
        file.close();
        // TODO: create a default config for basic functionality in the event the file is not found
    }
    file.close();
    config["CB_baseAddress"] = this->address;
    config["CB_port"] = this->port;

    // here is where we call the setup function in the BTManager
    this->client = new httplib::Client(BT_MANAGER_ADDRESS);
    this->client_id = "none";
    httplib::Result res = this->client->Post("/setup", config.dump(), "application/json");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
    } else if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
    } else {
        CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
        this->client_id = res->body;
    }

    // TODO: Setup all the callback endpoints
    this->server->Get("/device_connected", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string mac = req.get_param_value("mac");
        std::string device = "";
        if (req.has_param("device")) {
            device = req.get_param_value("device");
        }
        bool paired = false;
        if (req.has_param("paired")) {
            paired = req.get_param_value("paired") == "true";
        }
        std::string rssi = "";
        if (req.has_param("rssi")) {
            rssi = req.get_param_value("rssi");
        }
        std::string alias = "";
        if (req.has_param("alias")) {
            alias = req.get_param_value("alias");
        }
        std::string manufacturer = "";
        if (req.has_param("manufacturer")) {
            manufacturer = req.get_param_value("manufacturer");
        }
        bool connected = false;
        if (req.has_param("connected")) {
            connected = req.get_param_value("connected") == "true";
        }
        bool trusted = false;
        if (req.has_param("trusted")) {
            trusted = req.get_param_value("trusted") == "true";
        }
        bool blocked = false;
        if (req.has_param("blocked")) {
            blocked = req.get_param_value("blocked") == "true";
        }
        BTDevice dev;
        dev.name = device;
        dev.mac = mac;
        dev.paired = paired;
        dev.rssi = rssi;
        dev.alias = alias;
        dev.manufacturer = manufacturer;
        dev.connected = connected;
        dev.trusted = trusted;
        dev.blocked = blocked;
        // verify that the device is not already in the list
        for (auto d : this->devices) {
            if (d.mac == dev.mac) {
                return;
            }
        }
        this->devices.push_back(dev);
        if (dev.connected) {
            this->connectedDevices.push_back(dev);
        }
        if (dev.paired) {
            this->pairedDevices.push_back(dev);
        }
        if (dev.connected && dev.paired) {
            this->availableDevices.push_back(dev);
        }
        std::string response = "Device added to devices list. ";
        if (dev.connected) {
            response += "Device added to connected list. ";
        }
        if (dev.paired) {
            response += "Device added to paired list. ";
        }
        if (dev.connected && dev.paired) {
            response += "Device added to available list. ";
        }
        res.set_content(response, "text/plain");
    });

    this->server->Get("/device_disconnected", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string mac = req.get_param_value("mac");
        std::string device = "";
        if (req.has_param("device")) {
            device = req.get_param_value("device");
        }
        bool paired = false;
        if (req.has_param("paired")) {
            paired = req.get_param_value("paired") == "true";
        }
        std::string rssi = "";
        if (req.has_param("rssi")) {
            rssi = req.get_param_value("rssi");
        }
        std::string alias = "";
        if (req.has_param("alias")) {
            alias = req.get_param_value("alias");
        }
        std::string manufacturer = "";
        if (req.has_param("manufacturer")) {
            manufacturer = req.get_param_value("manufacturer");
        }
        bool connected = false;
        if (req.has_param("connected")) {
            connected = req.get_param_value("connected") == "true";
        }
        bool trusted = false;
        if (req.has_param("trusted")) {
            trusted = req.get_param_value("trusted") == "true";
        }
        bool blocked = false;
        if (req.has_param("blocked")) {
            blocked = req.get_param_value("blocked") == "true";
        }
        BTDevice dev;
        dev.name = device;
        dev.mac = mac;
        dev.paired = paired;
        dev.rssi = rssi;
        dev.alias = alias;
        dev.manufacturer = manufacturer;
        dev.connected = connected;
        dev.trusted = trusted;
        dev.blocked = blocked;
        // verify that the device is not already in the list
        bool found = false;
        for (auto d : this->devices) {
            if (d.mac == dev.mac) {
                found = true;
            }
        }
        if (!found)
            this->devices.push_back(dev);
        std::string response = "Device added to devices list. ";
        if (!dev.connected) {
            // remove the device from the connected list
            for (auto it = this->connectedDevices.begin(); it != this->connectedDevices.end(); it++) {
                if (it->mac == dev.mac) {
                    this->connectedDevices.erase(it);
                    break;
                }
            }
        }
        res.set_content(response, "text/plain");
    });

    this->server->Get("/device_paired", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string mac = req.get_param_value("mac");
        std::string device = "";
        if (req.has_param("device")) {
            device = req.get_param_value("device");
        }
        bool paired = false;
        if (req.has_param("paired")) {
            paired = req.get_param_value("paired") == "true";
        }
        std::string rssi = "";
        if (req.has_param("rssi")) {
            rssi = req.get_param_value("rssi");
        }
        std::string alias = "";
        if (req.has_param("alias")) {
            alias = req.get_param_value("alias");
        }
        std::string manufacturer = "";
        if (req.has_param("manufacturer")) {
            manufacturer = req.get_param_value("manufacturer");
        }
        bool connected = false;
        if (req.has_param("connected")) {
            connected = req.get_param_value("connected") == "true";
        }
        bool trusted = false;
        if (req.has_param("trusted")) {
            trusted = req.get_param_value("trusted") == "true";
        }
        bool blocked = false;
        if (req.has_param("blocked")) {
            blocked = req.get_param_value("blocked") == "true";
        }
        BTDevice dev;
        dev.name = device;
        dev.mac = mac;
        dev.paired = paired;
        dev.rssi = rssi;
        dev.alias = alias;
        dev.manufacturer = manufacturer;
        dev.connected = connected;
        dev.trusted = trusted;
        dev.blocked = blocked;
        // verify that the device is not already in the list
        bool found = false;
        for (auto d : this->devices) {
            if (d.mac == dev.mac) {
                found = true;
            }
        }
        if (!found)
            this->devices.push_back(dev);
        std::string response = "Device added to devices list. ";
        if (!dev.paired) {
            // remove the device from the paired list
            for (auto it = this->pairedDevices.begin(); it != this->pairedDevices.end(); it++) {
                if (it->mac == dev.mac) {
                    this->pairedDevices.erase(it);
                    break;
                }
            }
        } else {
            // add the device to the paired list
            this->pairedDevices.push_back(dev);
        }
        res.set_content(response, "text/plain");
    });

    this->server->Get("/device_unpaired", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string mac = req.get_param_value("mac");
        std::string device = "";
        if (req.has_param("device")) {
            device = req.get_param_value("device");
        }
        bool paired = false;
        if (req.has_param("paired")) {
            paired = req.get_param_value("paired") == "true";
        }
        std::string rssi = "";
        if (req.has_param("rssi")) {
            rssi = req.get_param_value("rssi");
        }
        std::string alias = "";
        if (req.has_param("alias")) {
            alias = req.get_param_value("alias");
        }
        std::string manufacturer = "";
        if (req.has_param("manufacturer")) {
            manufacturer = req.get_param_value("manufacturer");
        }
        bool connected = false;
        if (req.has_param("connected")) {
            connected = req.get_param_value("connected") == "true";
        }
        bool trusted = false;
        if (req.has_param("trusted")) {
            trusted = req.get_param_value("trusted") == "true";
        }
        bool blocked = false;
        if (req.has_param("blocked")) {
            blocked = req.get_param_value("blocked") == "true";
        }
        BTDevice dev;
        dev.name = device;
        dev.mac = mac;
        dev.paired = paired;
        dev.rssi = rssi;
        dev.alias = alias;
        dev.manufacturer = manufacturer;
        dev.connected = connected;
        dev.trusted = trusted;
        dev.blocked = blocked;
        // verify that the device is not already in the list
        bool found = false;
        for (auto d : this->devices) {
            if (d.mac == dev.mac) {
                found = true;
            }
        }
        if (!found)
            this->devices.push_back(dev);
        std::string response = "Device added to devices list. ";
        if (!dev.paired) {
            // remove the device from the paired list
            for (auto it = this->pairedDevices.begin(); it != this->pairedDevices.end(); it++) {
                if (it->mac == dev.mac) {
                    this->pairedDevices.erase(it);
                    break;
                }
            }
        }
        res.set_content(response, "text/plain");
    });

    this->server->Get("/device_discovered", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string mac = req.get_param_value("mac");
        std::string device = "";
        if (req.has_param("device")) {
            device = req.get_param_value("device");
        }
        bool paired = false;
        if (req.has_param("paired")) {
            paired = req.get_param_value("paired") == "true";
        }
        std::string rssi = "";
        if (req.has_param("rssi")) {
            rssi = req.get_param_value("rssi");
        }
        std::string alias = "";
        if (req.has_param("alias")) {
            alias = req.get_param_value("alias");
        }
        std::string manufacturer = "";
        if (req.has_param("manufacturer")) {
            manufacturer = req.get_param_value("manufacturer");
        }
        bool connected = false;
        if (req.has_param("connected")) {
            connected = req.get_param_value("connected") == "true";
        }
        bool trusted = false;
        if (req.has_param("trusted")) {
            trusted = req.get_param_value("trusted") == "true";
        }
        bool blocked = false;
        if (req.has_param("blocked")) {
            blocked = req.get_param_value("blocked") == "true";
        }
        BTDevice dev;
        dev.name = device;
        dev.mac = mac;
        dev.paired = paired;
        dev.rssi = rssi;
        dev.alias = alias;
        dev.manufacturer = manufacturer;
        dev.connected = connected;
        dev.trusted = trusted;
        dev.blocked = blocked;
        // verify that the device is not already in the list
        bool found = false;
        for (auto d : this->devices) {
            if (d.mac == dev.mac) {
                found = true;
            }
        }
        if (!found)
            this->devices.push_back(dev);
        std::string response = "Device added to devices list. ";
        if (dev.connected) {
            this->connectedDevices.push_back(dev);
        }
        if (dev.paired) {
            this->pairedDevices.push_back(dev);
        }
        if (dev.connected && dev.paired) {
            this->availableDevices.push_back(dev);
        }
        res.set_content(response, "text/plain");
    });

    this->server->Get("/connection_error", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("error")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string error = req.get_param_value("error");
        CubeLog::error("Connection error: " + error);
        res.set_content("ok", "text/plain");
    });

    this->server->Get("/pairing_request", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac") || !req.has_param("device")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string device = req.get_param_value("device");
        std::string mac = req.get_param_value("mac");

        std::string passkey = "";
        if (req.has_param("passkey")) {
            passkey = req.get_param_value("passkey");
        }
        std::string pin = "";
        if (req.has_param("pin")) {
            pin = req.get_param_value("pin");
        }

        CubeLog::info("Pairing request from: " + device + " with mac: " + mac);
        if (!passkey.empty()) {
            CubeLog::info("Passkey: " + passkey);
        }
        if (!pin.empty()) {
            CubeLog::info("Pin: " + pin);
        }
        // TODO: need to prompt the user to accept the pairing request. This will require a call to the appropriate GUI function.
        // Once the user has accepted the pairing request, we will need to call the appropriate function in the BTManager to accept the pairing request.
        res.set_content("ok", "text/plain");
    });

    this->server->Get("/pairing_success", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac") || !req.has_param("device")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string device = req.get_param_value("device");
        std::string mac = req.get_param_value("mac");

        CubeLog::info("Pairing success from: " + device + " with mac: " + mac);
        // TODO: notify the user
        res.set_content("ok", "text/plain");
    });

    this->server->Get("/pairing_failure", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("mac") || !req.has_param("device")) {
            res.set_content("Missing parameter", "text/plain");
            CubeLog::error("device_connected called: missing parameter.");
            return;
        }
        std::string device = req.get_param_value("device");
        std::string mac = req.get_param_value("mac");

        CubeLog::info("Pairing failure from: " + device + " with mac: " + mac);
        // TODO: notify the user
        res.set_content("ok", "text/plain");
    });

    this->server->Get("/bluetooth_enabled", [&](const httplib::Request& req, httplib::Response& res) {
        CubeLog::info("Bluetooth enabled");
        // TODO: update the Global setting for bluetooth enabled
        res.set_content("ok", "text/plain");
    });

    this->server->Get("/bluetooth_disabled", [&](const httplib::Request& req, httplib::Response& res) {
        CubeLog::info("Bluetooth disabled");
        // TODO: update the Global setting for bluetooth enabled
        res.set_content("ok", "text/plain");
    });

    // this->server->Get("/scan_started") // TODO:
    // this->server->Get("/scan_stopped")
    // this->server->Get("/bonding_started")
    // this->server->Get("/bonding_completed")
    // this->server->Get("/bonding_failed")

    // Start the server
    this->serverThread = new std::jthread([&] {
#ifdef _WIN32
        this->server->listen(this->address, this->port);
#else
        this->server->set_address_family(AF_UNIX).listen(this->address, 80);
#endif
    });

    // Start the heartbeat thread
    this->heartbeatThread = new std::jthread([&, config](std::stop_token st) {
        nlohmann::json j;
        while (!st.stop_requested()) {
            genericSleep(10 * 1000);
            if (this->client_id == "none") {
                CubeLog::info("Client id not set. Attempting to get client id.");
                httplib::Result res = this->client->Post("/setup", config.dump(), "application/json");
                if (!res) {
                    continue;
                }
                if (res->status != 200) {
                    continue;
                }
                this->client_id = res->body;
                continue;
            }
            j["status"] = "alive";
            j["client_id"] = this->client_id;
            httplib::Result res = this->client->Post("/heartbeat", j.dump(), "application/json");
            if (!res) {
                CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/heartbeat");
                this->client_id = "none";
                continue;
            }
            if (res->status != 200) {
                CubeLog::error("Response code: " + std::to_string(res->status));
                this->client_id = "none";
                continue;
            }
            if (res->body != "ok") {
                CubeLog::error("Heartbeat failed: " + res->body);
                this->client_id = "none";
            }
        }
        j["status"] = "shutdown";
        j["client_id"] = this->client_id;
        this->client->Post("/heartbeat", j.dump(), "application/json");
    });
}

BTControl::~BTControl()
{
    this->server->stop();
    delete this->server;
    delete this->serverThread;
    delete this->heartbeatThread;
}

bool BTControl::scanForDevices()
{
    httplib::Result res = this->client->Get("/scan");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/scan");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + "/scan");
    if (res->body == "ok") {
        return true;
    }
    return false;
}

bool BTControl::stopScanning()
{
    httplib::Result res = this->client->Get("/stop_scan");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/stop_scan");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/stop_scan");
    if (res->body == "ok") {
        return true;
    }
    return false;
}

bool BTControl::makeVisible(bool visible)
{
    httplib::Result res = this->client->Get("/make_visible?visible=" + std::string((visible ? "true" : "false")));
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/make_visible");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/make_visible");
    if (res->body == "ok") {
        return true;
    }
    return false;
}

bool BTControl::connectToDevice(BTDevice& device)
{
    httplib::Result res = this->client->Post("/connect", device.mac, "text/plain");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/connect");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/connect");
    if (res->body == "ok") {
        return true;
    }
    return false;
}

bool BTControl::disconnectFromDevice(BTDevice& device)
{
    httplib::Result res = this->client->Post("/disconnect", device.mac, "text/plain");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/disconnect");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/disconnect");
    if (res->body == "ok") {
        return true;
    }
    return false;
}

bool BTControl::pairWithDevice(BTDevice& device)
{
    httplib::Result res = this->client->Post("/pair", device.mac, "text/plain");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/pair");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/pair");
    if (res->body == "ok") {
        return true;
    }
    return false;
}

std::vector<BTDevice> BTControl::getDevices()
{
    httplib::Result res = this->client->Get("/devices");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/devices");
        return std::vector<BTDevice>();
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return std::vector<BTDevice>();
    }
    nlohmann::json resJson = nlohmann::json::parse(res->body);
    std::vector<BTDevice> devices;
    for (auto d : resJson) {
        BTDevice dev;
        dev.name = d["name"];
        dev.mac = d["mac"];
        dev.paired = d["paired"];
        dev.rssi = d["rssi"];
        dev.alias = d["alias"];
        dev.manufacturer = d["manufacturer"];
        dev.connected = d["connected"];
        dev.trusted = d["trusted"];
        dev.blocked = d["blocked"];
        devices.push_back(dev);
    }
    return devices;
}

std::vector<BTDevice> BTControl::getPairedDevices()
{
    httplib::Result res = this->client->Get("/paired_devices");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/paired_devices");
        return std::vector<BTDevice>();
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return std::vector<BTDevice>();
    }
    nlohmann::json resJson = nlohmann::json::parse(res->body);
    std::vector<BTDevice> devices;
    for (auto d : resJson) {
        BTDevice dev;
        dev.name = d["name"];
        dev.mac = d["mac"];
        dev.paired = d["paired"];
        dev.rssi = d["rssi"];
        dev.alias = d["alias"];
        dev.manufacturer = d["manufacturer"];
        dev.connected = d["connected"];
        dev.trusted = d["trusted"];
        dev.blocked = d["blocked"];
        devices.push_back(dev);
    }
    return devices;
}

std::vector<BTDevice> BTControl::getConnectedDevices()
{
    httplib::Result res = this->client->Get("/connected_devices");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/connected_devices");
        return std::vector<BTDevice>();
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return std::vector<BTDevice>();
    }
    nlohmann::json resJson = nlohmann::json::parse(res->body);
    std::vector<BTDevice> devices;
    for (auto d : resJson) {
        BTDevice dev;
        dev.name = d["name"];
        dev.mac = d["mac"];
        dev.paired = d["paired"];
        dev.rssi = d["rssi"];
        dev.alias = d["alias"];
        dev.manufacturer = d["manufacturer"];
        dev.connected = d["connected"];
        dev.trusted = d["trusted"];
        dev.blocked = d["blocked"];
        devices.push_back(dev);
    }
    return devices;
}

std::vector<BTDevice> BTControl::getAvailableDevices()
{
    httplib::Result res = this->client->Get("/available_devices");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/available_devices");
        return std::vector<BTDevice>();
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return std::vector<BTDevice>();
    }
    nlohmann::json resJson = nlohmann::json::parse(res->body);
    std::vector<BTDevice> devices;
    for (auto d : resJson) {
        BTDevice dev;
        dev.name = d["name"];
        dev.mac = d["mac"];
        dev.paired = d["paired"];
        dev.rssi = d["rssi"];
        dev.alias = d["alias"];
        dev.manufacturer = d["manufacturer"];
        dev.connected = d["connected"];
        dev.trusted = d["trusted"];
        dev.blocked = d["blocked"];
        devices.push_back(dev);
    }
    return devices;
}

//////////////////////////////////////////////////////////////////////////////////

int BTService::_port = 55291;

BTService::BTService(const std::string& serviceName)
{
    // If we are on windows, the http server for this service will use the port as normal.
    // If we are on Linux, the http server will use a unix socket and will append the port number to the address
    // in order to ensure we have a unique socket for each service.
    this->serviceName = serviceName;
    this->port = BTService::_port++;
    this->server = new httplib::Server();
#ifdef _WIN32
    this->address = "0.0.0.0";
#else
    this->address = "/tmp/cube/bt_service_" + std::to_string(this->port) + ".sock";
#endif

    this->config["CB_baseAddress"] = this->address;
    this->config["CB_port"] = this->port;
    this->config["CB_serviceName"] = serviceName;
    this->client = new httplib::Client(BT_MANAGER_ADDRESS);
    httplib::Result res = this->client->Post("/setup", this->config.dump(), "application/json");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
        return;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
    this->client_id = res->body;
}

BTService::~BTService()
{
    this->server->stop();
    delete this->server;
    delete this->serverThread;
    delete this->heartbeatThread;
    delete this->client;
}

void BTService::start()
{
    this->characteristicsLocked = true;
    this->serverThread = new std::jthread([&] {
#ifdef _WIN32
        this->server->bind_to_port(this->address.c_str(), this->port);
        this->server->listen_after_bind();
#else
        this->server->set_address_family(AF_UNIX).listen(this->address, 80);
#endif
    });

    // Start the heartbeat thread
    this->heartbeatThread = new std::jthread([&](std::stop_token st) {
        nlohmann::json j;
        while (!st.stop_requested()) {
            genericSleep(10 * 1000);
            if (this->client_id == "none") {
                CubeLog::info("Client id not set. Attempting to get client id.");
                httplib::Result res = this->client->Post("/setup", this->config.dump(), "application/json");
                if (!res) {
                    continue;
                }
                if (res->status != 200) {
                    continue;
                }
                this->client_id = res->body;
                continue;
            }
            j["status"] = "alive";
            j["client_id"] = this->client_id;
            httplib::Result res = this->client->Post("/heartbeat", j.dump(), "application/json");
            if (!res) {
                CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/heartbeat");
                this->client_id = "none";
                continue;
            }
            if (res->status != 200) {
                CubeLog::error("Response code: " + std::to_string(res->status));
                this->client_id = "none";
                continue;
            }
            if (res->body != "ok") {
                CubeLog::error("Heartbeat failed: " + res->body);
                this->client_id = "none";
            }
        }
        j["status"] = "shutdown";
        j["client_id"] = this->client_id;
        this->client->Post("/heartbeat", j.dump(), "application/json");
    });
}

void BTService::addCharacteristic(const std::string& name, std::function<void(std::string)> callback)
{
    if (this->characteristicsLocked) {
        CubeLog::error("Characteristics are locked. Cannot add characteristic.");
        return;
    }
}

//////////////////////////////////////////////////////////////////////////////////

BTManager::BTManager()
{
    this->control = new BTControl();
    // TODO: create all the services that we will need to provide

    // TODO: check to see if the BTManager application is running. If it is not, we will need to start it.
}

BTManager::~BTManager()
{
    delete this->control;
}

HttpEndPointData_t BTManager::getHttpEndpointData()
{
    HttpEndPointData_t data;
    return data;
}

std::string BTManager::getInterfaceName() const
{
    return "Bluetooth";
}