#pragma once
#include <logger.h>
#include <nlohmann/json.hpp>
#include <globalSettings.h>

LogVerbosity GlobalSettings::logVerbosity = LogVerbosity::TIMESTAMP_AND_LEVEL_AND_FILE_AND_LINE_AND_FUNCTION_AND_NUMBEROFLOGS;
std::vector<std::string> GlobalSettings::fontPaths = {
    "fonts/Roboto/Roboto-Regular.ttf",
    "fonts/Roboto/Roboto-Bold.ttf",
    "fonts/Roboto/Roboto-Italic.ttf",
    "fonts/Roboto/Roboto-BoldItalic.ttf",
    "fonts/Roboto/Roboto-Light.ttf",
    "fonts/Roboto/Roboto-LightItalic.ttf",
    "fonts/Roboto/Roboto-Medium.ttf",
    "fonts/Roboto/Roboto-MediumItalic.ttf",
    "fonts/Roboto/Roboto-Thin.ttf",
    "fonts/Roboto/Roboto-ThinItalic.ttf",
    "fonts/Roboto/Roboto-Black.ttf",
    "fonts/Roboto/Roboto-BlackItalic.ttf",
    "fonts/Assistant/Assistant-Regular.ttf",
    "fonts/Assistant/Assistant-Bold.ttf",
    "fonts/Assistant/Assistant-ExtraBold.ttf",
    "fonts/Assistant/Assistant-ExtraLight.ttf",
    "fonts/Assistant/Assistant-Light.ttf",
    "fonts/Assistant/Assistant-Medium.ttf",
    "fonts/Assistant/Assistant-SemiBold.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-Bold.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-BoldItalic.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-Italic.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-Medium.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-MediumItalic.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-Regular.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-SemiBold.ttf",
    "fonts/RadioCanadaBig/RadioCanadaBig-SemiBoldItalic.ttf"
};
std::string GlobalSettings::selectedFontPath = GlobalSettings::fontPaths[GlobalSettings::FontPathIndices::ROBOTO_REGULAR].c_str();
LogLevel GlobalSettings::logLevelPrint = LogLevel::LOGGER_INFO;
LogLevel GlobalSettings::logLevelFile = LogLevel::LOGGER_INFO;