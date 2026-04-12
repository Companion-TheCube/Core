#include "../../src/settings/globalSettings.h"
#include <gtest/gtest.h>

namespace {

TEST(GlobalSettingsTest, ThermalDefaultsAreInitialized)
{
    GlobalSettings defaults;

    EXPECT_TRUE(GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::FAN_CONTROL_ENABLED));
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS), 2000);
    EXPECT_DOUBLE_EQ(GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::FAN_CONTROL_HYSTERESIS_C), 2.0);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT), 100);
    EXPECT_EQ(
        GlobalSettings::getSetting(GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS),
        nlohmann::json::array({
            nlohmann::json::array({ 35.0, 25 }),
            nlohmann::json::array({ 50.0, 40 }),
            nlohmann::json::array({ 65.0, 70 }),
            nlohmann::json::array({ 80.0, 100 }),
        }));
}

TEST(GlobalSettingsTest, ThermalNumericSettingsAreClamped)
{
    GlobalSettings defaults;

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS, 100);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS), 250);

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS, 20000);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS), 10000);

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_HYSTERESIS_C, -5.0);
    EXPECT_DOUBLE_EQ(GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::FAN_CONTROL_HYSTERESIS_C), 0.0);

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_HYSTERESIS_C, 25.0);
    EXPECT_DOUBLE_EQ(GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::FAN_CONTROL_HYSTERESIS_C), 10.0);

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT, -1);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT), 0);

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT, 150);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT), 100);
}

TEST(GlobalSettingsTest, ThermalCurvePointsAreSortedAndClamped)
{
    GlobalSettings defaults;

    GlobalSettings::setSetting(
        GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS,
        nlohmann::json::array({
            nlohmann::json::array({ 130.0, 120 }),
            nlohmann::json::array({ -10.0, -5 }),
            nlohmann::json::array({ 65.0, 70 }),
        }));

    EXPECT_EQ(
        GlobalSettings::getSetting(GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS),
        nlohmann::json::array({
            nlohmann::json::array({ 0.0, 0 }),
            nlohmann::json::array({ 65.0, 70 }),
            nlohmann::json::array({ 120.0, 100 }),
        }));
}

TEST(GlobalSettingsTest, InvalidThermalCurveFallsBackToDefaults)
{
    GlobalSettings defaults;

    GlobalSettings::setSetting(
        GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS,
        nlohmann::json::array({
            nlohmann::json::array({ 40.0, 20 }),
        }));

    EXPECT_EQ(
        GlobalSettings::getSetting(GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS),
        nlohmann::json::array({
            nlohmann::json::array({ 35.0, 25 }),
            nlohmann::json::array({ 50.0, 40 }),
            nlohmann::json::array({ 65.0, 70 }),
            nlohmann::json::array({ 80.0, 100 }),
        }));

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS, "not-an-array");
    EXPECT_EQ(
        GlobalSettings::getSetting(GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS),
        nlohmann::json::array({
            nlohmann::json::array({ 35.0, 25 }),
            nlohmann::json::array({ 50.0, 40 }),
            nlohmann::json::array({ 65.0, 70 }),
            nlohmann::json::array({ 80.0, 100 }),
        }));
}

} // namespace
