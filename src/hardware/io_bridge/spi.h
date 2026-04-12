/*
███████╗██████╗ ██╗   ██╗  ██╗
██╔════╝██╔══██╗██║   ██║  ██║
███████╗██████╔╝██║   ███████║
╚════██║██╔═══╝ ██║   ██╔══██║
███████║██║     ██║██╗██║  ██║
╚══════╝╚═╝     ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

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
This file defines the app-facing SPI endpoint wrapper for devices connected through the IO bridge.
SPI represents bridge SPI endpoint access, not Linux host SPI device access, and depends on IoBridgeSession for transport.
*/

#pragma once
#ifndef SPI_H
#define SPI_H

#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include <utils.h>

#ifndef LOGGER_H
#include <logger.h>
#endif
#ifndef API_I_H
#include "../api/api.h"
#endif

#include "ioBridge.h"

using base64String = std::string;

enum class SPIMode {
    MODE_0 = 0, // CPOL=0, CPHA=0
    MODE_1 = 1, // CPOL=0, CPHA=1
    MODE_2 = 2, // CPOL=1, CPHA=0
    MODE_3 = 3 // CPOL=1, CPHA=1
};

enum class SPIError {
    NONE = 0,
    NOT_INITIALIZED,
    HANDLE_NOT_FOUND,
    TRANSFER_FAILED,
    INVALID_ARGUMENT,
    TIMEOUT,
    UNKNOWN_ERROR,
    HANDLE_ALREADY_REGISTERED,
    INVALID_HANDLE,
    INVALID_SPEED,
    INVALID_MODE,
    SPI_NOT_SUPPORTED,
    SPI_TRANSFER_ERROR,
    SPI_INIT_ERROR,
    SPI_UNSUPPORTED_OPERATION,
    SPI_NOT_IMPLEMENTED,
    SPI_INVALID_RESPONSE,
    SPI_INVALID_DATA,
    SPI_UNEXPECTED_ERROR,
    SPI_PERMISSION_DENIED,
    SPI_DEVICE_NOT_FOUND,
    SPI_DEVICE_BUSY,
    SPI_DEVICE_NOT_READY,
    SPI_DEVICE_NOT_OPEN,
    SPI_DEVICE_NOT_CONFIGURED,
    SPI_DEVICE_NOT_AVAILABLE,
    SPI_DEVICE_NOT_SUPPORTED,
    SPI_DEVICE_NOT_INITIALIZED,
    SPI_DEVICE_ALREADY_INITIALIZED,
    SPI_DEVICE_NOT_CONNECTED,
    SPI_DEVICE_NOT_RESPONDING,
    SPI_DEVICE_NOT_READY_FOR_TRANSFER,
    SPI_DEVICE_NOT_READY_FOR_CONFIG,
};

/**
 * @brief Bridge-backed SPI endpoint wrapper for app-facing expansion devices.
 * This class stores per-handle SPI settings and forwards transfers through the shared IoBridgeSession.
 */
class SPI : public AutoRegisterAPI<SPI> {
public:
    SPI();
    explicit SPI(std::shared_ptr<IoBridgeSession> bridgeSession);
    ~SPI();
    void attachBridgeSession(std::shared_ptr<IoBridgeSession> bridgeSession);
    bool hasBridgeSession() const;
    /**
     * @brief Register settings for a bridge-managed SPI endpoint handle.
     * @param handle The requested handle for the bridge SPI endpoint.
     * @param speed The speed of the SPI communication.
     * @param mode The mode of the SPI communication.
     * @return 0 when the handle was stored successfully.
     */
    std::expected<int, SPIError> registerHandle(const std::string& handle, int speed, int mode);
    /**
     * @brief Transfer data over SPI.
     * @param handle The handle of the SPI device to communicate with.
     * @param txData The data to send (as a base64 encoded string).
     * @param rxLen The length of the expected response.
     * @return std::optional<std::string> The response data as a base64 encoded string, or an empty optional if the transfer failed.
     */
    std::optional<base64String> transferTxRx(const std::string& handle, const base64String& txData, size_t rxLen);
    /**
     * @brief Get the SPI settings for a specific bridge handle.
     * @param handle The handle of the bridge SPI endpoint.
     * @return A JSON object containing the SPI settings.
     */
    nlohmann::json getSettings(const std::string& handle);
    /**
     * @brief Get the list of registered SPI handles.
     * @return A vector of strings containing the names of the registered SPI handles.
     */
    std::vector<std::string> getRegisteredHandles();
    /**
     * @brief Transfer data over bridge-backed SPI without expecting a response.
     * @param handle The handle of the bridge SPI endpoint to communicate with.
     * @param txData The data to send (as a base64 encoded string).
     * @return true if the transfer was successful, false otherwise.
     */
    std::optional<base64String> transferTx(const std::string& handle, const std::string& txData);

private:
    /**
     * @brief Internal method to perform the bridge SPI transfer.
     * @param handle The handle of the bridge SPI endpoint.
     * @param txData The data to send (as a base64 encoded string).
     * @param rxLen The length of the expected response.
     * @return std::optional<std::string> The response data as a base64 encoded string, or an empty optional if the transfer failed.
     */
    std::optional<base64String> transfer(const std::string& handle, const base64String& txData, size_t rxLen);
    // Mutex for thread safety
    std::mutex spiMutex;
    // Map to store registered SPI handles and their settings
    std::unordered_map<std::string, nlohmann::json> spiHandles;
    // Internal method to check if a handle is registered
    bool isHandleRegistered(const std::string& handle) const;
    // Internal method to get the SPI settings for a handle
    nlohmann::json getHandleSettings(const std::string& handle);
    std::shared_ptr<IoBridgeSession> bridgeSession_;

    // I_API_Interface implementation
    std::string getInterfaceName() const override { return "SPI"; }
    HttpEndPointData_t getHttpEndpointData() override;
};
#endif
