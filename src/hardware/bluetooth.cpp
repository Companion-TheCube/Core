/*
██████╗ ██╗     ██╗   ██╗███████╗████████╗ ██████╗  ██████╗ ████████╗██╗  ██╗    ██████╗██████╗ ██████╗ 
██╔══██╗██║     ██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔═══██╗╚══██╔══╝██║  ██║   ██╔════╝██╔══██╗██╔══██╗
██████╔╝██║     ██║   ██║█████╗     ██║   ██║   ██║██║   ██║   ██║   ███████║   ██║     ██████╔╝██████╔╝
██╔══██╗██║     ██║   ██║██╔══╝     ██║   ██║   ██║██║   ██║   ██║   ██╔══██║   ██║     ██╔═══╝ ██╔═══╝ 
██████╔╝███████╗╚██████╔╝███████╗   ██║   ╚██████╔╝╚██████╔╝   ██║   ██║  ██║██╗╚██████╗██║     ██║     
╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝  ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

/*

████████╗ ██████╗ ██████╗  ██████╗ 
╚══██╔══╝██╔═══██╗██╔══██╗██╔═══██╗
   ██║   ██║   ██║██║  ██║██║   ██║
   ██║   ██║   ██║██║  ██║██║   ██║
   ██║   ╚██████╔╝██████╔╝╚██████╔╝
   ╚═╝    ╚═════╝ ╚═════╝  ╚═════╝ 
                                   
TODO: since the BT_Manager app is far from complete, we should just mock the bluetooth functions for now.
*/


#include "bluetooth.h"


void startMock(std::stop_token st);
bool launchProcess(const std::string& execPath, const std::string& execArgs);
bool launchProcess(const std::string& execPath, const std::string& execArgs, std::string& result);

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

BTControl::BTControl(uuids::uuid authUUID)
{
    this->authUUID = authUUID;
    // First thing to do is see if the BTManager is running
    // If it is, we will connect to it and get the client_id
    // If it is not, we will start it and then connect to it
#ifndef PRODUCTION_BUILD
    mockThread = new std::jthread(startMock);
#endif
    this->server = new httplib::Server();
#ifdef _WIN32
    this->address = "0.0.0.0";
    this->port = 55285;
    // TODO: Need to check if the port is available
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
    this->client->set_default_headers({
        {"Authorization", "Bearer " + uuids::to_string(this->authUUID)}
    });
    httplib::Result res = this->client->Post("/setup", config.dump(), "application/json");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
    } else if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
    } else {
        CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
        nlohmann::json j = nlohmann::json::parse(res->body);
        if (j.contains("client_id")) {
            this->client_id = j["client_id"];
        }else{
            CubeLog::error("Error getting client_id from response: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
        }
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

    // TODO: register the callbacks with the GlobalSettings class so that the enabled/disable/etc actions get called when the setting is changed.

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
        unsigned long counter = 0;
        while (!st.stop_requested()) {
            genericSleep(100);
            counter++;
            if (counter % 100 != 0) {
                continue;
            }
            if (this->client_id == "none") {
                CubeLog::info("Client id not set. Attempting to get client id.");
                httplib::Result res = this->client->Post("/setup", config.dump(), "application/json");
                if (!res) {
                    continue;
                }
                if (res->status != 200) {
                    continue;
                }
                nlohmann::json j = nlohmann::json::parse(res->body);
                if (j.contains("client_id")) {
                    this->client_id = j["client_id"];
                }
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
#ifndef PRODUCTION_BUILD
    this->mockThread->request_stop();
    this->mockThread->join();
#endif
    if(this->server != nullptr){
        this->server->stop();
        delete this->server;
    }
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
    httplib::Result res = this->client->Get("/set_visible?visible=" + std::string((visible ? "true" : "false")));
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/set_visible");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/set_visible");
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
        if(d.contains("name"))
            dev.name = d["name"];
        if(d.contains("mac"))
            dev.mac = d["mac"];
        if(d.contains("paired"))
            dev.paired = d["paired"];
        if(d.contains("rssi"))
            dev.rssi = d["rssi"];
        if(d.contains("alias"))
            dev.alias = d["alias"];
        if(d.contains("manufacturer"))
            dev.manufacturer = d["manufacturer"];
        if(d.contains("connected"))
            dev.connected = d["connected"];
        if(d.contains("trusted"))
            dev.trusted = d["trusted"];
        if(d.contains("blocked"))
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
        if(d.contains("name"))
            dev.name = d["name"];
        if(d.contains("mac"))
            dev.mac = d["mac"];
        if(d.contains("paired"))
            dev.paired = d["paired"];
        if(d.contains("rssi"))
            dev.rssi = d["rssi"];
        if(d.contains("alias"))
            dev.alias = d["alias"];
        if(d.contains("manufacturer"))
            dev.manufacturer = d["manufacturer"];
        if(d.contains("connected"))
            dev.connected = d["connected"];
        if(d.contains("trusted"))
            dev.trusted = d["trusted"];
        if(d.contains("blocked"))
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
        if(d.contains("name"))
            dev.name = d["name"];
        if(d.contains("mac"))
            dev.mac = d["mac"];
        if(d.contains("paired"))
            dev.paired = d["paired"];
        if(d.contains("rssi"))
            dev.rssi = d["rssi"];
        if(d.contains("alias"))
            dev.alias = d["alias"];
        if(d.contains("manufacturer"))
            dev.manufacturer = d["manufacturer"];
        if(d.contains("connected"))
            dev.connected = d["connected"];
        if(d.contains("trusted"))
            dev.trusted = d["trusted"];
        if(d.contains("blocked"))
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
        if(d.contains("name"))
            dev.name = d["name"];
        if(d.contains("mac"))
            dev.mac = d["mac"];
        if(d.contains("paired"))
            dev.paired = d["paired"];
        if(d.contains("rssi"))
            dev.rssi = d["rssi"];
        if(d.contains("alias"))
            dev.alias = d["alias"];
        if(d.contains("manufacturer"))
            dev.manufacturer = d["manufacturer"];
        if(d.contains("connected"))
            dev.connected = d["connected"];
        if(d.contains("trusted"))
            dev.trusted = d["trusted"];
        if(d.contains("blocked"))
            dev.blocked = d["blocked"];
        devices.push_back(dev);
    }
    return devices;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BTService::BTService(const nlohmann::json& config, httplib::Server* server, uuids::uuid authUUID)
{
    this->authUUID = authUUID;
    this->config = config;
    this->server = server;
    this->client = new httplib::Client(BT_MANAGER_ADDRESS);
    this->client->set_default_headers({
        {"Authorization", "Bearer " + uuids::to_string(this->authUUID)}
    });
    this->client_id = "none";
}

BTService::~BTService()
{
    delete this->client;
}

// To be called once all the characteristics have been added
bool BTService::start()
{
    this->characteristicsLocked = true;
    httplib::Result res = this->client->Post("/setup", this->config.dump(), "application/json");
    if (!res) {
        CubeLog::error("Error getting response: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
        return false;
    }
    if (res->status != 200) {
        CubeLog::error("Response code: " + std::to_string(res->status));
        return false;
    }
    CubeLog::info("Response: " + res->body + "From: " + std::string(BT_MANAGER_ADDRESS) + "/setup");
    nlohmann::json j = nlohmann::json::parse(res->body);
    if (j.contains("client_id")) {
        this->client_id = j["client_id"];
    }
    if(this->client_id == "none"){
        CubeLog::error("Client id not set. Cannot create service.");
        return false;
    }
    return true;
}

void BTService::addCharacteristic(const std::string& name, uuids::uuid uuid, std::function<void(std::string)> callback)
{
    if (this->characteristicsLocked) {
        CubeLog::error("Characteristics are locked. Cannot add characteristic.");
        return;
    }
    // TODO: we need to add this characteristic to config json for this service

    auto characteristic = new BTCharacteristic(name);
    characteristic->uuid = uuid;
    characteristic->callback_void_string = callback;
    characteristic->cbType = BTCharacteristic::CBType::VOID_STRING;
}

//////////////////////////////////////////////////////////////////////////////////

BTManager::BTManager()
{
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size> {};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);
    uuids::uuid_random_generator gen{generator};
    this->authUUID = gen();

    // check to see if the BTManager application is running. If it is not, we will need to start it.
    // Get running processes
#ifdef _WIN32
    std::string command = "tasklist";
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    // WCHAR tempCommand[128];
    // convertStringToWCHAR(command, tempCommand);
    if(!CreateProcess(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CubeLog::error("Error starting process: " + command);
        return;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (exitCode != 0) {
        CubeLog::error("Error starting process: " + command);
        return;
    }
    char buffer[128];
    DWORD bytesRead;
    std::string result = "";
    while (ReadFile(pi.hProcess, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        result += std::string(buffer, bytesRead);
    }
#else
    std::string command = "ps | grep ";
    command += BT_MANAGER_EXECUTABLE;
    std::string result = "";
    launchProcess(command, "", result);
#endif
    
    if (result.find(BT_MANAGER_EXECUTABLE) == std::string::npos) {
        // TODO: this section depends on the BTManager application being completed. We will need to update this section
        // once the BTManager application is completed.
        // The BTManager is not running
        // Start it and then connect to it
        // TODO: Start the BTManager. The command line args for this should include the address for an http endpoint
        // that will provide an authentication key to the BTManager. We will need to include this key in all requests to the BTManager.
        // The BTManager should also have the -s flag set so that only one instance can run at a time. (--single_instance)
        // httplib::Server authServer;
        // bool authGotten = false;
        // authServer.Get("/auth", [&](const httplib::Request& req, httplib::Response& res) {
        //     res.set_content(this->authUUID.str(), "text/plain");
        //     authGotten = true;
        // });
        // std::jthread serverThread([&] {
        //      authServer.listen("localhost", 55300);
        // });
        // genericSleep(1000); // wait for the authServer to start
        // std::string command = BT_MANAGER_EXECUTABLE;
        // std::string args = "-s --auth_cb \"localhost:55300/auth\"";
        // if(!launchProcess(command, args)){
        //    CubeLog::error("Error starting process: " + command + " " + args);
        //    authServer.stop();
        //    return;
        // }
        // while(!authGotten){
        //     genericSleep(10);
        // }
        // genericSleep(100); // ensure that the server finished sending the auth key
        // authServer.stop(); // stop the server now that we have the auth key
    }

    // This class will handle the server for all the BTService class instances
    // It will also handle the heartbeat for the BTManager

    this->control = new BTControl(this->authUUID);
    this->server = new httplib::Server();
    this->client = new httplib::Client(BT_MANAGER_ADDRESS);
    this->client_id = "none";

    this->config = nlohmann::json::object();
    this->config["name"] = "BTManager";
    this->config["address"] = "localhost";
    this->config["port"] = 80;
    this->config["characteristics_client_ids"] = nlohmann::json::array();
    

    // TODO: add all the services that we want to use
    // This should probably be a member function of this class since we'll want to be able to add services via the API.
    // We should also define all the services in a json file that will use the same format that apps can use via the API.
    // Then all we have to do is load the json file and create the services.
    // BTService service1(config, this->server);
    // this->config["characteristics_client_ids"].push_back("service1_client_id");

    // TODO: this->client_id needs a mutex. When this class is destroyed, we need to make sure we can get a lock on that mutex
    // so that we don't destroy the client_id while the heartbeat thread is still using it.

    this->heartbeatThread = new std::jthread([&](std::stop_token st) {
        nlohmann::json j;
        while (!st.stop_requested()) {
            genericSleep(10 * 1000);
            if(st.stop_requested()){
                break;
            }
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

    // TODO: The server will need to listen for any incoming requests from the BTManager application and handle them.
    // Requests can be either GET or POST and the address will have to be parsed to determine the action to take.
    // Addresses will be in the form of /<client_id>/<action>?<params>
    // <client_id> will be the client_id of one of the BTService instances stored in the services vector.
    // Iterating through the services vector and checking the getClientId() function will allow us to determine the service to use.
    // <action> will be the name of action to take on the BTService instance and are stored as 
    // std::vector<BTCharacteristic*> characteristics in the BTService instance.
    // <params> will be the parameters to pass to the action. These will be in the form of key=value pairs and are only applicable to
    // GET requests. POST requests will have the parameters in the body of the request as JSON.
    // for(auto c : service.getCharacteristics()){
    //     if(c->getName() == action){
    //         // do action. Actions may be any of BTCharacteristic::CBType and we will need to call the appropriate function
    //         // on the BTService instance. Parameters will be parsed from either the address or the body of the request.
    //     }
    // }
    // this->server->GET("/:client_id/:action", [&](const httplib::Request& req, httplib::Response& res) {
    //     std::string client_id = req.path_params.at("client_id");
    //     std::string action = req.path_params.at("action");
    //     std::string params = req.params;
    //     for(auto s : this->services){
    //         if(s.getClientId() == client_id){
    //             for(auto c : s.getCharacteristics()){
    //                 if(c->getName() == action){
    //                     // do action // Probably should break this part out to a function that will parse the params and call the appropriate function.
    //                 }
    //             }
    //         }
    //     }
    //     res.set_content("ok", "text/plain"); // This will need to be changed to an appropriate response based on the action taken.
    // });
    // this->server->POST("/:client_id/:action", [&](const httplib::Request& req, httplib::Response& res) {
    //     std::string client_id = req.path_params.at("client_id");
    //     std::string action = req.path_params.at("action");
    //     nlohmann::json j = nlohmann::json::parse(req.body);
    //     for(auto s : this->services){
    //         if(s.getClientId() == client_id){
    //             for(auto c : s.getCharacteristics()){
    //                 if(c->getName() == action){
    //                     // do action // Probably should break this part out to a function that will parse the params and call the appropriate function.
    //                 }
    //             }
    //         }
    //     }
    //     res.set_content("ok", "text/plain"); // This will need to be changed to an appropriate response based on the action taken.
    // });

    this->serverThread = new std::jthread([&] {
#ifdef _WIN32
        //this->server->listen(this->config["address"], this->config["port"]);
#else
        //this->server->set_address_family(AF_UNIX).listen(this->config["address"], this->config["port"]);
#endif
    });
}

BTManager::~BTManager()
{
    delete this->control;
}

HttpEndPointData_t BTManager::getHttpEndpointData()
{
    HttpEndPointData_t data;
    EndpointAction_t action;
    data.push_back({
        GET_ENDPOINT | PRIVATE_ENDPOINT,
        [this](const httplib::Request& req, httplib::Response& res) {
            // TODO: This should shutdown the BTManager class enough to allow adding new BT Services.
            nlohmann::json j;
            j["client_id"] = this->client_id;
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR , "");
        },
        "/stopBTManager",
        {},
        "Stops the BTManager class and allows adding new BT Services."
    });
    data.push_back({
        GET_ENDPOINT | PRIVATE_ENDPOINT,
        [this](const httplib::Request& req, httplib::Response& res) {
            // TODO: This should start the BTManager class after it has been stopped.
            nlohmann::json j;
            j["client_id"] = this->client_id;
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR , "");
        },
        "/startBTManager",
        {},
        "Starts the BTManager class after it has been stopped."
    });
    data.push_back({
        POST_ENDPOINT | PRIVATE_ENDPOINT,
        [this](const httplib::Request& req, httplib::Response& res) {
            // TODO: This should provide a way to add a new BT Service to the BTManager.
            nlohmann::json j;
            j["client_id"] = this->client_id;
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR , "");
            /*
            Adding a new service will allow apps to interact with the BT_manager application. 
            When an app provides it config via this endpoint, it can have the callback endpoint(s)
            that are sent to the manager app point directly back to the app. This way, an app can 
            get data from BT devices without the CORE having to be involved. We can also add
            an endpoint here that allows an app to get the auth key for the BT_manager app. This
            way, the app can use the auth key to communicate with the BT_manager app directly.
            */
        },
        "/addBTService",
        {},
        "Adds a new BT Service to the BTManager."
    });
    return data;
}

constexpr std::string BTManager::getInterfaceName() const
{
    return "Bluetooth";
}

//////////////////////////////////////////////////////////////////////////////////

void startMock(std::stop_token st){
    httplib::Server server;
    server.Get("/setup", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["client_id"] = "mock";
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/setup", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["client_id"] = "mock";
        res.set_content(j.dump(), "application/json");
    });

    server.Get("/heartbeat", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("ok", "text/plain");
    });

    server.Post("/heartbeat", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("ok", "text/plain");
    });

    server.Get("/scan", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });

    server.Get("/stop_scan", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });

    server.Get("/make_visible", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/connect", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/disconnect", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/pair", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });

    server.Get("/devices", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j.push_back({{"name", "device1"}, {"mac", "00:00:00:00:00:00"}, {"paired", true}, {"rssi", "-50"}, {"alias", "device1"}, {"manufacturer", "manufacturer1"}, {"connected", true}, {"trusted", true}, {"blocked", false}});
        j.push_back({{"name", "device2"}, {"mac", "00:00:00:00:00:01"}, {"paired", false}, {"rssi", "-60"}, {"alias", "device2"}, {"manufacturer", "manufacturer2"}, {"connected", false}, {"trusted", false}, {"blocked", false}});
        j.push_back({{"name", "device3"}, {"mac", "00:00:00:00:00:02"}, {"paired", true}, {"rssi", "-70"}, {"alias", "device3"}, {"manufacturer", "manufacturer3"}, {"connected", false}, {"trusted", true}, {"blocked", false}});
        res.set_content(j.dump(), "application/json");
    });

    server.Get("/paired_devices", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j.push_back({{"name", "device1"}, {"mac", "00:00:00:00:00:00"}, {"paired", true}, {"rssi", "-50"}, {"alias", "device1"}, {"manufacturer", "manufacturer1"}, {"connected", true}, {"trusted", true}, {"blocked", false}});
        j.push_back({{"name", "device3"}, {"mac", "00:00:00:00:00:02"}, {"paired", true}, {"rssi", "-70"}, {"alias", "device3"}, {"manufacturer", "manufacturer3"}, {"connected", false}, {"trusted", true}, {"blocked", false}});
        res.set_content(j.dump(), "application/json");
    });

    server.Get("/connected_devices", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j.push_back({{"name", "device1"}, {"mac", "00:00:00:00:00:00"}, {"paired", true}, {"rssi", "-50"}, {"alias", "device1"}, {"manufacturer", "manufacturer1"}, {"connected", true}, {"trusted", true}, {"blocked", false}});
        res.set_content(j.dump(), "application/json");
    });

    server.Get("/available_devices", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j;
        j.push_back({{"name", "device1"}, {"mac", "00:00:00:00:00:00"}, {"paired", true}, {"rssi", "-50"}, {"alias", "device1"}, {"manufacturer", "manufacturer1"}, {"connected", true}, {"trusted", true}, {"blocked", false}});
        res.set_content(j.dump(), "application/json");
    });

    std::jthread t = std::jthread([&]{
        server.listen("localhost", 55290);
    });
    while(!st.stop_requested()){
        genericSleep(500);
    }
    server.stop();
}

bool launchProcess(const std::string& execPath, const std::string& execArgs, std::string& result)
{
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path fullPath = cwd / execPath;
    if (!std::filesystem::exists(fullPath)) {
        CubeLog::error("App not found: " + fullPath.string());
        return false;
    }
#ifdef _WIN32
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hStdOutRead = NULL;
    HANDLE hStdOutWrite = NULL;
    HANDLE hStdErrRead = NULL;
    HANDLE hStdErrWrite = NULL;
    HANDLE hStdInRead = NULL;
    HANDLE hStdInWrite = NULL;

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
        CubeLog::error("Stdout pipe creation failed");
        return false;
    }
    if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stdout SetHandleInformation failed");
        return false;
    }
    if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0)) {
        CubeLog::error("Stderr pipe creation failed");
        return false;
    }
    if (!SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stderr SetHandleInformation failed");
        return false;
    }
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0)) {
        CubeLog::error("Stdin pipe creation failed");
        return false;
    }
    if (!SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stdin SetHandleInformation failed");
        return false;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    si.hStdError = hStdErrWrite;
    si.hStdOutput = hStdOutWrite;
    si.hStdInput = hStdInRead;
    si.dwFlags |= STARTF_USESTDHANDLES;

    cwd = std::filesystem::current_path().string();
    std::string execCommand = execPath + " " + execArgs;
    // WCHAR tempCommand[128];
    // convertStringToWCHAR(execCommand, tempCommand);
    if (!CreateProcess(NULL,
            (LPSTR)execCommand.c_str(), // command line
            NULL, // process security attributes
            NULL, // primary thread security attributes
            TRUE, // handles are inherited
            0, // creation flags
            NULL, // use parent's environment
            NULL, // use parent's current directory
            &si, // STARTUPINFO pointer
            &pi)) { // receives PROCESS_INFORMATION
        CubeLog::error("Error: " + std::to_string(GetLastError()));
        return false;
    } else {
        CubeLog::info("App started: " + execPath + " " + execArgs);
        CubeLog::info("Process ID: " + std::to_string(pi.dwProcessId));
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
#endif
#ifdef __linux__
    pid_t pid = 0;
    std::string execCommand = execPath + " " + execArgs;
    CubeLog::debug("Exec command: " + execCommand);
    const char* path = execPath.c_str();
    std::vector<std::string> args;
    // split execArgs by space
    std::istringstream iss(execArgs);
    for (std::string s; iss >> s;) {
        args.push_back(s);
    }
    char* argv[args.size() + 2];
    argv[0] = (char*)path;
    for (size_t i = 0; i < args.size(); i++) {
        argv[i + 1] = (char*)args[i].c_str();
    }
    argv[args.size() + 1] = NULL;
    // add the current working directory to the path
    std::string path_str = cwd /  execPath;

    // setup stdout and stderr
    int stdoutPipe[2];
    if (pipe(stdoutPipe) == -1) {
        CubeLog::error("Failed to create stdout pipe");
        return false;
    }
    int stderrPipe[2];
    if (pipe(stderrPipe) == -1) {
        CubeLog::error("Failed to create stderr pipe");
        return false;
    }
    int stdinPipe[2];
    if (pipe(stdinPipe) == -1) {
        CubeLog::error("Failed to create stdin pipe");
        return false;
    }
    posix_spawn_file_actions_t* actions;
    posix_spawn_file_actions_init(actions);

    posix_spawn_file_actions_adddup2(actions, stdoutPipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(actions, stderrPipe[1], STDERR_FILENO);
    posix_spawn_file_actions_adddup2(actions, stdinPipe[0], STDIN_FILENO);
    posix_spawn_file_actions_addclose(actions, stdoutPipe[0]);
    posix_spawn_file_actions_addclose(actions, stderrPipe[0]);
    posix_spawn_file_actions_addclose(actions, stdinPipe[1]);
    posix_spawn_file_actions_addclose(actions, stdoutPipe[1]);
    posix_spawn_file_actions_addclose(actions, stderrPipe[1]);
    posix_spawn_file_actions_addclose(actions, stdinPipe[0]);

    if(posix_spawn(&pid, path, actions, NULL, argv, environ) != 0){
        CubeLog::error("Failed to spawn process: " + execCommand);
        return false;
    }
    close(stdoutPipe[1]);
    close(stderrPipe[1]);
    close(stdinPipe[0]);
    char buffer[128];
    ssize_t bytesRead;
    while((bytesRead = read(stdoutPipe[0], buffer, sizeof(buffer))) != 0){
        result += std::string(buffer, bytesRead);
    }
    close(stdoutPipe[0]);
    close(stderrPipe[0]);
    close(stdinPipe[1]);
    return true;
#endif
}

bool launchProcess(const std::string& execPath, const std::string& execArgs)
{
    CubeLog::info("Starting app: " + execPath + " " + execArgs);
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path fullPath = cwd / execPath;
    if (!std::filesystem::exists(fullPath)) {
        CubeLog::error("App not found: " + fullPath.string());
        return false;
    }
#ifdef _WIN32
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hStdOutRead = NULL;
    HANDLE hStdOutWrite = NULL;
    HANDLE hStdErrRead = NULL;
    HANDLE hStdErrWrite = NULL;
    HANDLE hStdInRead = NULL;
    HANDLE hStdInWrite = NULL;

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
        CubeLog::error("Stdout pipe creation failed");
        return false;
    }
    if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stdout SetHandleInformation failed");
        return false;
    }
    if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0)) {
        CubeLog::error("Stderr pipe creation failed");
        return false;
    }
    if (!SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stderr SetHandleInformation failed");
        return false;
    }
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0)) {
        CubeLog::error("Stdin pipe creation failed");
        return false;
    }
    if (!SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0)) {
        CubeLog::error("Stdin SetHandleInformation failed");
        return false;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    si.hStdError = hStdErrWrite;
    si.hStdOutput = hStdOutWrite;
    si.hStdInput = hStdInRead;
    si.dwFlags |= STARTF_USESTDHANDLES;

    cwd = std::filesystem::current_path().string();
    std::string execCommand = execPath + " " + execArgs;
    // WCHAR tempCommand[128];
    // convertStringToWCHAR(execCommand, tempCommand);
    if (!CreateProcess(NULL,
            (LPSTR)execCommand.c_str(), // command line
            NULL, // process security attributes
            NULL, // primary thread security attributes
            TRUE, // handles are inherited
            0, // creation flags
            NULL, // use parent's environment
            NULL, // use parent's current directory
            &si, // STARTUPINFO pointer
            &pi)) { // receives PROCESS_INFORMATION
        CubeLog::error("Error: " + std::to_string(GetLastError()));
        return false;
    } else {
        CubeLog::info("App started: " + execPath + " " + execArgs);
        CubeLog::info("Process ID: " + std::to_string(pi.dwProcessId));
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
#endif
#ifdef __linux__
    pid_t pid = 0;
    std::string execCommand = execPath + " " + execArgs;
    CubeLog::debug("Exec command: " + execCommand);
    const char* path = execPath.c_str();
    std::vector<std::string> args;
    // split execArgs by space
    std::istringstream iss(execArgs);
    for (std::string s; iss >> s;) {
        args.push_back(s);
    }
    char* argv[args.size() + 2];
    argv[0] = (char*)path;
    for (size_t i = 0; i < args.size(); i++) {
        argv[i + 1] = (char*)args[i].c_str();
    }
    argv[args.size() + 1] = NULL;
    // add the current working directory to the path
    std::string path_str = cwd /  execPath;

    // setup stdout and stderr
    int stdoutPipe[2];
    if (pipe(stdoutPipe) == -1) {
        CubeLog::error("Failed to create stdout pipe");
        return false;
    }
    int stderrPipe[2];
    if (pipe(stderrPipe) == -1) {
        CubeLog::error("Failed to create stderr pipe");
        return false;
    }
    int stdinPipe[2];
    if (pipe(stdinPipe) == -1) {
        CubeLog::error("Failed to create stdin pipe");
        return false;
    }
    posix_spawn_file_actions_t* actions;
    posix_spawn_file_actions_init(actions);

    posix_spawn_file_actions_adddup2(actions, stdoutPipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(actions, stderrPipe[1], STDERR_FILENO);
    posix_spawn_file_actions_adddup2(actions, stdinPipe[0], STDIN_FILENO);
    posix_spawn_file_actions_addclose(actions, stdoutPipe[0]);
    posix_spawn_file_actions_addclose(actions, stdoutPipe[1]);
    posix_spawn_file_actions_addclose(actions, stderrPipe[0]);

    int status = posix_spawn(&pid, path_str.c_str(), actions, NULL, const_cast<char* const*>(argv), environ);

    if (status != 0) {
        CubeLog::error("Failed to start app: " + execPath + " " + execArgs);
        return false;
    }
    // close the write end of the pipes
    close(stdoutPipe[1]);
    close(stderrPipe[1]);
    close(stdinPipe[0]);
    CubeLog::debug("Process created with PID: " + std::to_string(pid));
    CubeLog::info("App started: " + execPath + " " + execArgs);
    return true;
#endif
#ifndef _WIN32
#ifndef __linux__
    CubeLog::error("Unsupported platform");
    return false;
#endif
#endif
}