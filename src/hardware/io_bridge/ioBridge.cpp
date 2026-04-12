/*
██╗ ██████╗ ██████╗ ██████╗ ██╗██████╗  ██████╗ ███████╗    ██████╗██████╗ ██████╗
██║██╔═══██╗██╔══██╗██╔══██╗██║██╔══██╗██╔════╝ ██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██║██║   ██║██████╔╝██████╔╝██║██║  ██║██║  ███╗█████╗     ██║     ██████╔╝██████╔╝
██║██║   ██║██╔══██╗██╔══██╗██║██║  ██║██║   ██║██╔══╝     ██║     ██╔═══╝ ██╔═══╝
██║╚██████╔╝██████╔╝██║  ██║██║██████╔╝╚██████╔╝███████╗██╗╚██████╗██║     ██║
╚═╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝╚═════╝  ╚═════╝ ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
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
This file implements the host-side IO bridge session abstraction used by app-facing bridge endpoints.
IoBridgeSession centralizes access to the underlying bridge transport so SPI, I2C, UART, and GPIO endpoint wrappers can share one coordinator.
*/

#include "ioBridge.h"

IoBridgeSession::IoBridgeSession(std::shared_ptr<IIoBridgeTransport> transport)
    : transport_(std::move(transport))
{
}

void IoBridgeSession::attachTransport(std::shared_ptr<IIoBridgeTransport> transport)
{
    std::lock_guard<std::mutex> lock(mutex_);
    transport_ = std::move(transport);
}

bool IoBridgeSession::isAttached() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(transport_);
}

std::expected<IoBridgePayload, IoBridgeError> IoBridgeSession::transact(const IoBridgeTransaction& transaction) const
{
    std::shared_ptr<IIoBridgeTransport> transport;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        transport = transport_;
    }

    if (!transport) {
        return std::unexpected(IoBridgeError::NOT_CONNECTED);
    }

    return transport->transact(transaction);
}
