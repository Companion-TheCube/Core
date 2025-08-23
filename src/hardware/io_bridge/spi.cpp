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

The SPI class is responsible for managing SPI communication with the Cube hardware.
It provides methods to initialize the SPI interface, send and receive data, and handle
communication with the Cube's hardware components.

Apps will use an API endpoint to send commands to the Cube, which will then be processed by the SPI class.
Example payload for sending a command to the Cube:
App → CORE Req: {"op":"SPI.transfer","handle":"accel","tx":"AQAIAAA=", "rxLen": 6}
CORE ← App Res: {"ok":true,"rx":"..."}

Tx and Rx data is base64 encoded to ensure safe transmission over HTTP.
Apps will have to request a specific SPI handle to communicate with the Cube hardware. Each handle corresponds to a 
specific hardware component (e.g., accelerometer, gyroscope, etc.) with specific SPI settings.
The SPI class will handle the actual communication with the hardware, including setting up the SPI interface,
sending commands, and receiving responses.

There should be a separate SPI class for registering handles.

*/

#include "spi.h"

SPI::SPI()
{
    CubeLog::info("SPI class initialized");
}

SPI::~SPI()
{
    CubeLog::info("SPI class destroyed");
}

std::expected<int, SPIError> SPI::registerHandle(const std::string& handle, int speed, int mode)
{
    if (!sanitizeHandleString(handle)) {
        CubeLog::error("Invalid SPI handle name: " + handle);
        return std::unexpected(SPIError::INVALID_HANDLE);
    }
    if (speed <= 0) {
        CubeLog::error("Invalid SPI speed: " + std::to_string(speed));
        return std::unexpected(SPIError::INVALID_SPEED);
    }
    if (mode < 0 || mode > 3) {
        CubeLog::error("Invalid SPI mode: " + std::to_string(mode));
        return std::unexpected(SPIError::INVALID_MODE);
    }
    // Lock the mutex for thread safety
    std::lock_guard<std::mutex> lock(spiMutex);
    if (isHandleRegistered(handle)) {
        CubeLog::error("SPI handle already registered: " + handle);
        return std::unexpected(SPIError::HANDLE_ALREADY_REGISTERED);
    }

    if (!initializeSPI(handle, speed, mode)) {
        CubeLog::error("Failed to initialize SPI handle: " + handle);
        return std::unexpected(SPIError::NOT_INITIALIZED);
    }

    nlohmann::json settings;
    settings["speed"] = speed;
    settings["mode"] = mode;
    spiHandles[handle] = settings;

    CubeLog::info("SPI handle registered: " + handle);
    return 0; // Success
}

std::optional<base64String> SPI::transferTxRx(const std::string& handle, const base64String& txData, size_t rxLen)
{
    return transfer(handle, txData, rxLen);
}

nlohmann::json SPI::getSettings(const std::string& handle)
{
    std::lock_guard<std::mutex> lock(spiMutex);
    return getHandleSettings(handle);
}

std::vector<std::string> SPI::getRegisteredHandles()
{
    std::lock_guard<std::mutex> lock(spiMutex);
    std::vector<std::string> handles;
    for (const auto& pair : spiHandles) {
        handles.push_back(pair.first);
    }
    return handles;
}

std::optional<base64String> SPI::transferTx(const std::string& handle, const std::string& txData)
{
    return transfer(handle, txData, 0);
}

std::optional<base64String> SPI::transfer(const std::string& handle, const base64String& txData, size_t rxLen)
{
    std::lock_guard<std::mutex> lock(spiMutex);
    if (!isHandleRegistered(handle)) {
        CubeLog::error("SPI handle not registered: " + handle);
        return std::nullopt;
    }

    // Decode the base64 encoded txData
    std::vector<unsigned char> txBytes;
    try {
        txBytes = cppcodec::base64_rfc4648::decode(txData);
    } catch (const std::exception& e) {
        CubeLog::error("Failed to decode base64 txData: " + std::string(e.what()));
        return std::nullopt;
    }

    /*
    
    TODO:
    The code below interacts directly with the SPI device but we need to make it so that it instead interacts with
    the IO Bridge (which will then interact with the Pi's SPI device).

    The IO Bridge is an RP2354 IC connected via SPI that provides I2C, SPI, UART, GPIO, and other interfaces to the Cube.
    See ioBridge.h and ioBridge.cpp for more information.
    
    */

    uint8_t mode = spiHandles[handle]["mode"];
    int speed = spiHandles[handle]["speed"];
    // int fd = open(spiDevicePath.c_str(), O_RDWR | O_CLOEXEC);
    // if (fd < 0) {
    //     CubeLog::error("Failed to open SPI device: " + spiDevicePath);
    //     return std::nullopt;
    // }

    // // Set SPI mode
    // if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
    //     CubeLog::error("Failed to set SPI mode for handle: " + handle);
    //     close(fd);
    //     return std::nullopt;
    // }
    // // Read back SPI mode to verify
    // uint8_t modeRead = 0;
    // if (ioctl(fd, SPI_IOC_RD_MODE, &modeRead) == -1) {
    //     CubeLog::error("Failed to read back SPI mode for handle: " + handle);
    //     close(fd);
    //     return std::nullopt;
    // }
    // if (modeRead != mode) {
    //     CubeLog::error("SPI mode verification failed for handle: " + handle +
    //                    ". Written mode: " + std::to_string(mode) +
    //                    ", Read mode: " + std::to_string(modeRead));
    //     close(fd);
    //     return std::nullopt;
    // }

    // // Set SPI speed
    // if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
    //     CubeLog::error("Failed to set SPI speed for handle: " + handle);
    //     close(fd);
    //     return std::nullopt;
    // }

    // std::vector<unsigned char> rxBytes(rxLen, 0);

    // struct spi_ioc_transfer tr;
    // memset(&tr, 0, sizeof(tr));
    // tr.len = txBytes.size() > rxLen ? txBytes.size() : rxLen;
    // // if tx.len is longer than txBytes.size(), pad the rest with 0s
    // if (tr.len > txBytes.size()) {
    //     std::vector<unsigned char> paddedTx(tr.len, 0);
    //     std::copy(txBytes.begin(), txBytes.end(), paddedTx.begin());
    //     tr.tx_buf = static_cast<__u64>(reinterpret_cast<uintptr_t>(paddedTx.data()));
    // } else {
    //     tr.tx_buf = static_cast<__u64>(reinterpret_cast<uintptr_t>(txBytes.data()));
    // }
    // tr.rx_buf = static_cast<__u64>(reinterpret_cast<uintptr_t>(rxBytes.data()));
    // tr.speed_hz = speed;
    // tr.delay_usecs = 0;
    // tr.bits_per_word = 8;

    // if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
    //     CubeLog::error("Failed to perform SPI transfer for handle: " + handle);
    //     close(fd);
    //     return std::nullopt;
    // }

    // close(fd);

    // Encode the received data back to base64
    return base64_encode_cube(txBytes); // TODO: Placeholder: return txBytes as rxBytes for now 
}

bool SPI::setSpiDevicePath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(spiMutex);
    // Check if the path exists and is a character device
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        CubeLog::error("SPI device path does not exist: " + path);
        return false;
    }
    if (!S_ISCHR(st.st_mode)) {
        CubeLog::error("SPI device path is not a character device: " + path);
        return false;
    }
    spiDevicePath = path;
    CubeLog::info("SPI device path set to: " + path);
    return true;
}

nlohmann::json SPI::getHandleSettings(const std::string& handle)
{
    if (spiHandles.find(handle) != spiHandles.end()) {
        return spiHandles[handle];
    } else {
        CubeLog::error("SPI handle not found: " + handle);
        return nlohmann::json();
    }
}

bool SPI::isHandleRegistered(const std::string& handle) const
{
    return spiHandles.find(handle) != spiHandles.end();
}

bool SPI::initializeSPI(const std::string& handle, int speed, int mode)
{
    // For now, just check if the device path is set
    if (spiDevicePath.empty()) {
        CubeLog::error("SPI device path is not set. Cannot initialize handle: " + handle);
        return false;
    }
    // Additional initialization steps can be added here if needed
    return true;
}

HttpEndPointData_t SPI::getHttpEndpointData()
{
    HttpEndPointData_t endpoints;

    // Endpoint to register an SPI handle
    // endpoints.push_back(

    // Endpoint to transfer data over SPI with response
    endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_header("Content-Type") || req.get_header_value("Content-Type") != "application/json") {
                CubeLog::error("SPI.transfer called: invalid Content-Type header.");
                nlohmann::json j;
                j["ok"] = false;
                j["error"] = "Invalid Content-Type header. Expected application/json.";
                res.set_content(j.dump(), "application/json");
                res.status = 400;
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST, "Invalid Content-Type header");
            }
            nlohmann::json reqJson;
            try {
                reqJson = nlohmann::json::parse(req.body);
            } catch (const std::exception& e) {
                CubeLog::error("SPI.transfer called: failed to parse JSON body: " + std::string(e.what()));
                nlohmann::json j;
                j["ok"] = false;
                j["error"] = "Failed to parse JSON body: " + std::string(e.what());
                res.set_content(j.dump(), "application/json");
                res.status = 400;
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST, "Failed to parse JSON body");
            }
            if (!reqJson.contains("handle") || !reqJson["handle"].is_string()) {
                CubeLog::error("SPI.transfer called: missing or invalid 'handle' field.");
                nlohmann::json j;
                j["ok"] = false;
                j["error"] = "Missing or invalid 'handle' field.";
                res.set_content(j.dump(), "application/json");
                res.status = 400;
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Missing or invalid 'handle' field.");
            }
            if (!reqJson.contains("tx") || !reqJson["tx"].is_string()) {
                CubeLog::error("SPI.transfer called: missing or invalid 'tx' field.");
                nlohmann::json j;
                j["ok"] = false;
                j["error"] = "Missing or invalid 'tx' field.";
                res.set_content(j.dump(), "application/json");
                res.status = 400;
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Missing or invalid 'tx' field.");
            }
            if (!reqJson.contains("rxLen") || !reqJson["rxLen"].is_number_unsigned()) {
                CubeLog::error("SPI.transfer called: missing or invalid 'rxLen' field.");
                nlohmann::json j;
                j["ok"] = false;
                j["error"] = "Missing or invalid 'rxLen' field.";
                res.set_content(j.dump(), "application/json");
                res.status = 400;
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Missing or invalid 'rxLen' field.");
            }
            std::string handle = reqJson["handle"];
            std::string txData = reqJson["tx"];
            size_t rxLen = reqJson["rxLen"];
            auto result = transferTxRx(handle, txData, rxLen);
            if (!result) {
                CubeLog::error("SPI.transfer called: transfer failed for handle: " + handle);
                nlohmann::json j;
                j["ok"] = false;
                j["error"] = "SPI transfer failed for handle: " + handle;
                res.set_content(j.dump(), "application/json");
                res.status = 500;
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "SPI transfer failed");
            }
            nlohmann::json j;
            j["ok"] = true;
            j["rx"] = *result;
            res.set_content(j.dump(), "application/json");
            res.status = 200;
            CubeLog::info("SPI.transfer called: transfer successful for handle: " + handle);
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "SPI_transfer_txrx",
        nlohmann::json({ { "type", "object" }, { "properties", { } } }),
        "Transfer data over SPI and receive a response"
    });
    // TODO: Implement the other endpoints
    // Endpoint to transfer data over SPI without response
    // endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,

    // Endpoint to get SPI settings for a handle
    // endpoints.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,

    // Endpoint to get list of registered SPI handles
    // endpoints.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,

    return endpoints;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

