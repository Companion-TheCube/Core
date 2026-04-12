/*
██╗  ██╗ █████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ███████╗██╗███╗   ██╗███████╗ ██████╗     ██████╗██████╗ ██████╗
██║  ██║██╔══██╗██╔══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔════╝██║████╗  ██║██╔════╝██╔═══██╗   ██╔════╝██╔══██╗██╔══██╗
███████║███████║██████╔╝██║  ██║██║ █╗ ██║███████║██████╔╝█████╗  ██║██╔██╗ ██║█████╗  ██║   ██║   ██║     ██████╔╝██████╔╝
██╔══██║██╔══██║██╔══██╗██║  ██║██║███╗██║██╔══██║██╔══██╗██╔══╝  ██║██║╚██╗██║██╔══╝  ██║   ██║   ██║     ██╔═══╝ ██╔═══╝
██║  ██║██║  ██║██║  ██║██████╔╝╚███╔███╔╝██║  ██║██║  ██║███████╗██║██║ ╚████║██║     ╚██████╔╝██╗╚██████╗██║     ██║
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝  ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═══╝╚═╝      ╚═════╝ ╚═╝ ╚═════╝╚═╝     ╚═╝
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

#include "hardwareInfo.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

std::optional<std::string> defaultFileReader(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string trim(std::string value)
{
    auto notSpace = [](unsigned char ch) {
        return !std::isspace(ch);
    };

    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::optional<std::string> readTrimmedText(
    const std::filesystem::path& path,
    const HardwareInfo::FileReader& fileReader)
{
    const auto text = fileReader ? fileReader(path) : defaultFileReader(path);
    if (!text.has_value()) {
        return std::nullopt;
    }

    const std::string trimmed = trim(*text);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    return trimmed;
}

std::string normalizeLabel(const std::string& value)
{
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return normalized;
}

bool isCpuLikeLabel(const std::string& value)
{
    const std::string normalized = normalizeLabel(value);
    return normalized.find("cpu") != std::string::npos
        || normalized.find("package") != std::string::npos
        || normalized.find("soc") != std::string::npos;
}

std::optional<double> parseTemperatureCelsius(const std::string& rawValue)
{
    try {
        double parsed = std::stod(trim(rawValue));
        if (parsed > 200.0 || parsed < -100.0) {
            parsed /= 1000.0;
        }
        return parsed;
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<HardwareTemperatureReading> readThermalZoneSensors(
    const std::filesystem::path& sysRoot,
    const HardwareInfo::FileReader& fileReader)
{
    std::vector<HardwareTemperatureReading> sensors;
    const auto thermalRoot = sysRoot / "class" / "thermal";

    std::error_code ec;
    if (!std::filesystem::exists(thermalRoot, ec)) {
        return sensors;
    }

    for (const auto& entry : std::filesystem::directory_iterator(thermalRoot, ec)) {
        if (ec || !entry.is_directory()) {
            continue;
        }
        if (entry.path().filename().string().rfind("thermal_zone", 0) != 0) {
            continue;
        }

        const auto rawTemp = readTrimmedText(entry.path() / "temp", fileReader);
        if (!rawTemp.has_value()) {
            continue;
        }
        const auto celsius = parseTemperatureCelsius(*rawTemp);
        if (!celsius.has_value()) {
            continue;
        }

        const auto type = readTrimmedText(entry.path() / "type", fileReader);
        const std::string name = type.value_or(entry.path().filename().string());
        sensors.push_back({
            .name = name,
            .sourcePath = (entry.path() / "temp").string(),
            .celsius = *celsius,
            .cpuLike = isCpuLikeLabel(name)
        });
    }

    return sensors;
}

std::vector<HardwareTemperatureReading> readHwmonSensors(
    const std::filesystem::path& sysRoot,
    const HardwareInfo::FileReader& fileReader)
{
    std::vector<HardwareTemperatureReading> sensors;
    const auto hwmonRoot = sysRoot / "class" / "hwmon";

    std::error_code ec;
    if (!std::filesystem::exists(hwmonRoot, ec)) {
        return sensors;
    }

    for (const auto& entry : std::filesystem::directory_iterator(hwmonRoot, ec)) {
        if (ec || !entry.is_directory()) {
            continue;
        }
        if (entry.path().filename().string().rfind("hwmon", 0) != 0) {
            continue;
        }

        const auto hwmonName = readTrimmedText(entry.path() / "name", fileReader)
            .value_or(entry.path().filename().string());

        for (const auto& sensorFile : std::filesystem::directory_iterator(entry.path(), ec)) {
            if (ec || !sensorFile.is_regular_file()) {
                continue;
            }

            const std::string filename = sensorFile.path().filename().string();
            if (filename.rfind("temp", 0) != 0 || filename.find("_input") == std::string::npos) {
                continue;
            }

            const auto rawTemp = readTrimmedText(sensorFile.path(), fileReader);
            if (!rawTemp.has_value()) {
                continue;
            }
            const auto celsius = parseTemperatureCelsius(*rawTemp);
            if (!celsius.has_value()) {
                continue;
            }

            std::string labelFilename = filename;
            const auto inputSuffixPos = labelFilename.rfind("_input");
            labelFilename.replace(inputSuffixPos, std::string("_input").size(), "_label");

            const auto label = readTrimmedText(sensorFile.path().parent_path() / labelFilename, fileReader);
            const std::string name = label.has_value()
                ? *label
                : hwmonName + ":" + sensorFile.path().stem().string();

            sensors.push_back({
                .name = name,
                .sourcePath = sensorFile.path().string(),
                .celsius = *celsius,
                .cpuLike = isCpuLikeLabel(label.value_or(hwmonName))
            });
        }
    }

    return sensors;
}

std::optional<double> maxTemperatureForPredicate(
    const std::vector<HardwareTemperatureReading>& sensors,
    const std::function<bool(const HardwareTemperatureReading&)>& predicate)
{
    std::optional<double> maxValue;
    for (const auto& sensor : sensors) {
        if (!predicate(sensor)) {
            continue;
        }
        if (!maxValue.has_value() || sensor.celsius > *maxValue) {
            maxValue = sensor.celsius;
        }
    }
    return maxValue;
}

} // namespace

HardwareInfoSnapshot HardwareInfo::snapshot()
{
    return snapshot("/sys");
}

std::vector<HardwareTemperatureReading> HardwareInfo::thermalSensors()
{
    return thermalSensors("/sys");
}

std::optional<double> HardwareInfo::cpuTemperatureCelsius()
{
    return cpuTemperatureCelsius("/sys");
}

std::optional<double> HardwareInfo::systemTemperatureCelsius()
{
    return systemTemperatureCelsius("/sys");
}

HardwareInfoSnapshot HardwareInfo::snapshot(
    const std::filesystem::path& sysRoot,
    FileReader fileReader)
{
    HardwareInfoSnapshot info;
    info.infowareVersion = std::string(iware::version);
    info.osInfo = iware::system::OS_info();
    info.kernelInfo = iware::system::kernel_info();
    info.cpuArchitecture = iware::cpu::architecture();
    info.cpuEndianness = iware::cpu::endianness();
    info.cpuVendor = iware::cpu::vendor();
    info.cpuVendorId = iware::cpu::vendor_id();
    info.cpuModelName = iware::cpu::model_name();
    info.cpuFrequencyHz = iware::cpu::frequency();
    info.cpuQuantities = iware::cpu::quantities();
    info.memory = iware::system::memory();
    info.thermalSensors = thermalSensors(sysRoot, std::move(fileReader));
    info.cpuTemperatureC = maxTemperatureForPredicate(info.thermalSensors, [](const HardwareTemperatureReading& sensor) {
        return sensor.cpuLike;
    });
    info.systemTemperatureC = maxTemperatureForPredicate(info.thermalSensors, [](const HardwareTemperatureReading&) {
        return true;
    });
    return info;
}

std::vector<HardwareTemperatureReading> HardwareInfo::thermalSensors(
    const std::filesystem::path& sysRoot,
    FileReader fileReader)
{
    auto sensors = readThermalZoneSensors(sysRoot, fileReader);
    if (sensors.empty()) {
        sensors = readHwmonSensors(sysRoot, fileReader);
    }

    std::sort(sensors.begin(), sensors.end(), [](const HardwareTemperatureReading& left, const HardwareTemperatureReading& right) {
        return left.sourcePath < right.sourcePath;
    });
    return sensors;
}

std::optional<double> HardwareInfo::cpuTemperatureCelsius(
    const std::filesystem::path& sysRoot,
    FileReader fileReader)
{
    return snapshot(sysRoot, std::move(fileReader)).cpuTemperatureC;
}

std::optional<double> HardwareInfo::systemTemperatureCelsius(
    const std::filesystem::path& sysRoot,
    FileReader fileReader)
{
    return snapshot(sysRoot, std::move(fileReader)).systemTemperatureC;
}
