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

TEST(GlobalSettingsTest, InteractionDefaultsAreInitialized)
{
    GlobalSettings defaults;

    EXPECT_TRUE(GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::INTERACTION_DETECTION_ENABLED));
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_POLL_INTERVAL_MS), 50);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_EVENT_HISTORY_SIZE), 128);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_TAP_DEBOUNCE_MS), 120);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_LIFT_CONFIRM_MS), 150);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_REST_STABLE_MS), 500);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_LIFT_DELTA_THRESHOLD_MG), 200);
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

TEST(GlobalSettingsTest, InteractionNumericSettingsAreClamped)
{
    GlobalSettings defaults;

    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_POLL_INTERVAL_MS, 1);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_POLL_INTERVAL_MS), 20);

    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_EVENT_HISTORY_SIZE, 5000);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_EVENT_HISTORY_SIZE), 1024);

    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_TAP_DEBOUNCE_MS, 5);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_TAP_DEBOUNCE_MS), 50);

    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_LIFT_CONFIRM_MS, 2000);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_LIFT_CONFIRM_MS), 1000);

    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_REST_STABLE_MS, 50);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_REST_STABLE_MS), 100);

    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_LIFT_DELTA_THRESHOLD_MG, 5000);
    EXPECT_EQ(GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::INTERACTION_LIFT_DELTA_THRESHOLD_MG), 1000);
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
