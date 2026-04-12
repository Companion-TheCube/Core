#include "../../src/hardware/hardwareInfo.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace {

class TempDir {
public:
    TempDir()
    {
        path_ = std::filesystem::temp_directory_path() / std::filesystem::path("thecube-hwinfo-" + std::to_string(++counter_));
        std::filesystem::create_directories(path_);
    }

    ~TempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    const std::filesystem::path& path() const
    {
        return path_;
    }

private:
    static inline int counter_ = 0;
    std::filesystem::path path_;
};

void writeText(const std::filesystem::path& path, const std::string& value)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    file << value;
}

TEST(HardwareInfoTest, ReadsThermalZoneSensorsAndClassifiesCpuLikeReadings)
{
    TempDir tempDir;
    writeText(tempDir.path() / "class/thermal/thermal_zone0/type", "cpu-thermal\n");
    writeText(tempDir.path() / "class/thermal/thermal_zone0/temp", "45000\n");
    writeText(tempDir.path() / "class/thermal/thermal_zone1/type", "gpu\n");
    writeText(tempDir.path() / "class/thermal/thermal_zone1/temp", "30000\n");

    const auto sensors = HardwareInfo::thermalSensors(tempDir.path());

    ASSERT_EQ(sensors.size(), 2u);
    EXPECT_EQ(sensors[0].name, "cpu-thermal");
    EXPECT_DOUBLE_EQ(sensors[0].celsius, 45.0);
    EXPECT_TRUE(sensors[0].cpuLike);
    EXPECT_EQ(sensors[1].name, "gpu");
    EXPECT_DOUBLE_EQ(sensors[1].celsius, 30.0);
    EXPECT_FALSE(sensors[1].cpuLike);

    const auto cpuTemperatureC = HardwareInfo::cpuTemperatureCelsius(tempDir.path());
    ASSERT_TRUE(cpuTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*cpuTemperatureC, 45.0);

    const auto systemTemperatureC = HardwareInfo::systemTemperatureCelsius(tempDir.path());
    ASSERT_TRUE(systemTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*systemTemperatureC, 45.0);
}

TEST(HardwareInfoTest, FallsBackToHwmonSensorsWhenThermalZonesAreUnavailable)
{
    TempDir tempDir;
    writeText(tempDir.path() / "class/hwmon/hwmon0/name", "chip0\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/temp1_input", "38000\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/temp1_label", "Package id 0\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/temp2_input", "42000\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/temp2_label", "board\n");

    const auto sensors = HardwareInfo::thermalSensors(tempDir.path());

    ASSERT_EQ(sensors.size(), 2u);
    EXPECT_EQ(sensors[0].name, "Package id 0");
    EXPECT_TRUE(sensors[0].cpuLike);
    EXPECT_EQ(sensors[1].name, "board");
    EXPECT_FALSE(sensors[1].cpuLike);

    const auto cpuTemperatureC = HardwareInfo::cpuTemperatureCelsius(tempDir.path());
    ASSERT_TRUE(cpuTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*cpuTemperatureC, 38.0);

    const auto systemTemperatureC = HardwareInfo::systemTemperatureCelsius(tempDir.path());
    ASSERT_TRUE(systemTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*systemTemperatureC, 42.0);
}

TEST(HardwareInfoTest, PrefersThermalZonesOverHwmonFallback)
{
    TempDir tempDir;
    writeText(tempDir.path() / "class/thermal/thermal_zone0/type", "soc\n");
    writeText(tempDir.path() / "class/thermal/thermal_zone0/temp", "51000\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/name", "chip0\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/temp1_input", "65000\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/temp1_label", "Package id 0\n");

    const auto sensors = HardwareInfo::thermalSensors(tempDir.path());

    ASSERT_EQ(sensors.size(), 1u);
    EXPECT_EQ(sensors[0].name, "soc");
    EXPECT_DOUBLE_EQ(sensors[0].celsius, 51.0);
}

TEST(HardwareInfoTest, IgnoresMalformedOrUnreadableSensors)
{
    TempDir tempDir;
    writeText(tempDir.path() / "class/thermal/thermal_zone0/type", "cpu\n");
    writeText(tempDir.path() / "class/thermal/thermal_zone0/temp", "not-a-number\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/name", "chip0\n");
    writeText(tempDir.path() / "class/hwmon/hwmon0/temp1_input", "\n");

    const auto sensors = HardwareInfo::thermalSensors(tempDir.path());
    EXPECT_TRUE(sensors.empty());
    EXPECT_FALSE(HardwareInfo::cpuTemperatureCelsius(tempDir.path()).has_value());
    EXPECT_FALSE(HardwareInfo::systemTemperatureCelsius(tempDir.path()).has_value());
}

} // namespace
