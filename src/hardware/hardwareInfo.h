/*
██╗  ██╗ █████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ███████╗██╗███╗   ██╗███████╗ ██████╗    ██╗  ██╗
██║  ██║██╔══██╗██╔══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔════╝██║████╗  ██║██╔════╝██╔═══██╗   ██║  ██║
███████║███████║██████╔╝██║  ██║██║ █╗ ██║███████║██████╔╝█████╗  ██║██╔██╗ ██║█████╗  ██║   ██║   ███████║
██╔══██║██╔══██║██╔══██╗██║  ██║██║███╗██║██╔══██║██╔══██╗██╔══╝  ██║██║╚██╗██║██╔══╝  ██║   ██║   ██╔══██║
██║  ██║██║  ██║██║  ██║██████╔╝╚███╔███╔╝██║  ██║██║  ██║███████╗██║██║ ╚████║██║     ╚██████╔╝██╗██║  ██║
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝  ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═══╝╚═╝      ╚═════╝ ╚═╝╚═╝  ╚═╝
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
#ifndef HARDWAREINFO_H
#define HARDWAREINFO_H

#include "infoware/cpu.hpp"
#include "infoware/system.hpp"
#include "infoware/version.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct HardwareTemperatureReading {
    std::string name;
    std::string sourcePath;
    double celsius = 0.0;
    bool cpuLike = false;
};

struct HardwareInfoSnapshot {
    std::string infowareVersion;
    iware::system::OS_info_t osInfo {};
    iware::system::kernel_info_t kernelInfo {};
    iware::cpu::architecture_t cpuArchitecture = iware::cpu::architecture_t::unknown;
    iware::cpu::endianness_t cpuEndianness = iware::cpu::endianness_t::little;
    std::string cpuVendor;
    std::string cpuVendorId;
    std::string cpuModelName;
    std::uint64_t cpuFrequencyHz = 0;
    iware::cpu::quantities_t cpuQuantities {};
    iware::system::memory_t memory {};
    std::vector<HardwareTemperatureReading> thermalSensors;
    std::optional<double> cpuTemperatureC;
    std::optional<double> systemTemperatureC;
};

class HardwareInfo {
public:
    using FileReader = std::function<std::optional<std::string>(const std::filesystem::path&)>;

    static HardwareInfoSnapshot snapshot();
    static std::vector<HardwareTemperatureReading> thermalSensors();
    static std::optional<double> cpuTemperatureCelsius();
    static std::optional<double> systemTemperatureCelsius();

    static HardwareInfoSnapshot snapshot(
        const std::filesystem::path& sysRoot,
        FileReader fileReader = {});
    static std::vector<HardwareTemperatureReading> thermalSensors(
        const std::filesystem::path& sysRoot,
        FileReader fileReader = {});
    static std::optional<double> cpuTemperatureCelsius(
        const std::filesystem::path& sysRoot,
        FileReader fileReader = {});
    static std::optional<double> systemTemperatureCelsius(
        const std::filesystem::path& sysRoot,
        FileReader fileReader = {});
};

#endif // HARDWAREINFO_H
