/*
██╗ ██████╗ ██████╗ ██████╗ ██╗██████╗  ██████╗ ███████╗   ██╗  ██╗
██║██╔═══██╗██╔══██╗██╔══██╗██║██╔══██╗██╔════╝ ██╔════╝   ██║  ██║
██║██║   ██║██████╔╝██████╔╝██║██║  ██║██║  ███╗█████╗     ███████║
██║██║   ██║██╔══██╗██╔══██╗██║██║  ██║██║   ██║██╔══╝     ██╔══██║
██║╚██████╔╝██████╔╝██║  ██║██║██████╔╝╚██████╔╝███████╗██╗██║  ██║
╚═╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝╚═════╝  ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═╝
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
This file defines the host-side IO bridge session abstraction used by app-facing bridge endpoints.
IIoBridgeTransport is the low-level transport seam, and IoBridgeSession is the shared coordinator that endpoint wrappers depend on.
*/

#pragma once
#ifndef IOBRIDGE_H
#define IOBRIDGE_H

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <vector>

enum class IoBridgeEndpoint : uint8_t {
    Management = 0x00,
    FirmwareUpdate = 0x01,
    Pio = 0x02,
    Gpio = 0x10,
    I2C = 0x20,
    Spi = 0x21,
    Uart = 0x22,
    Can = 0x23,
    Interrupt = 0x30,
    Control = 0xFF,
};

enum class IoBridgeError {
    NOT_CONNECTED,
    INVALID_ARGUMENT,
    TRANSPORT_ERROR,
    TIMEOUT,
    INVALID_RESPONSE,
};

using IoBridgePayload = std::vector<unsigned char>;

struct IoBridgeTransaction {
    IoBridgeEndpoint endpoint = IoBridgeEndpoint::Management;
    IoBridgePayload payload;
    size_t expectedResponseLength = 0;
};

class IIoBridgeTransport {
public:
    virtual ~IIoBridgeTransport() = default;
    virtual std::expected<IoBridgePayload, IoBridgeError> transact(const IoBridgeTransaction& transaction) = 0;
};

class IoBridgeSession {
public:
    IoBridgeSession() = default;
    explicit IoBridgeSession(std::shared_ptr<IIoBridgeTransport> transport);

    void attachTransport(std::shared_ptr<IIoBridgeTransport> transport);
    bool isAttached() const;
    std::expected<IoBridgePayload, IoBridgeError> transact(const IoBridgeTransaction& transaction) const;

private:
    mutable std::mutex mutex_;
    std::shared_ptr<IIoBridgeTransport> transport_;
};

#endif
