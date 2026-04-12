/*
███╗   ███╗███╗   ███╗██╗    ██╗ █████╗ ██╗   ██╗███████╗███████╗███████╗██████╗ ██╗ █████╗ ██╗     ██╗     ██╗███████╗███████╗ ██████╗██╗   ██╗ ██████╗██╗     ███████╗   ██╗  ██╗
████╗ ████║████╗ ████║██║    ██║██╔══██╗██║   ██║██╔════╝██╔════╝██╔════╝██╔══██╗██║██╔══██╗██║     ██║     ██║██╔════╝██╔════╝██╔════╝╚██╗ ██╔╝██╔════╝██║     ██╔════╝   ██║  ██║
██╔████╔██║██╔████╔██║██║ █╗ ██║███████║██║   ██║█████╗  ███████╗█████╗  ██████╔╝██║███████║██║     ██║     ██║█████╗  █████╗  ██║      ╚████╔╝ ██║     ██║     █████╗     ███████║
██║╚██╔╝██║██║╚██╔╝██║██║███╗██║██╔══██║╚██╗ ██╔╝██╔══╝  ╚════██║██╔══╝  ██╔══██╗██║██╔══██║██║     ██║     ██║██╔══╝  ██╔══╝  ██║       ╚██╔╝  ██║     ██║     ██╔══╝     ██╔══██║
██║ ╚═╝ ██║██║ ╚═╝ ██║╚███╔███╔╝██║  ██║ ╚████╔╝ ███████╗███████║███████╗██║  ██║██║██║  ██║███████╗███████╗██║██║     ███████╗╚██████╗   ██║   ╚██████╗███████╗███████╗██╗██║  ██║
╚═╝     ╚═╝╚═╝     ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝╚═╝     ╚══════╝ ╚═════╝   ╚═╝    ╚═════╝╚══════╝╚══════╝╚═╝╚═╝  ╚═╝
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

#pragma once

#include <functional>
#include <string>
#include <vector>

struct MmWaveSerialConnectionAttempt {
    std::string path;
    int baud = -1;
    bool openSucceeded = false;
    bool signatureMatched = false;
};

struct MmWaveSerialConnectResult {
    int fd = -1;
    std::string path;
    int baud = -1;
    std::vector<MmWaveSerialConnectionAttempt> attempts;
};

struct MmWaveSerialBackend {
    std::function<int(const std::string&, int)> open;
    std::function<void(int)> close;
    std::function<bool(int, int)> probeSignature;
};

MmWaveSerialConnectResult connectMmWaveWithCandidates(
    const MmWaveSerialBackend& backend,
    const std::vector<std::string>& paths,
    const std::vector<int>& baudCandidates,
    int probeTimeoutMs);
