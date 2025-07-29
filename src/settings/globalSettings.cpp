/*
 ██████╗ ██╗      ██████╗ ██████╗  █████╗ ██╗     ███████╗███████╗████████╗████████╗██╗███╗   ██╗ ██████╗ ███████╗    ██████╗██████╗ ██████╗ 
██╔════╝ ██║     ██╔═══██╗██╔══██╗██╔══██╗██║     ██╔════╝██╔════╝╚══██╔══╝╚══██╔══╝██║████╗  ██║██╔════╝ ██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██║  ███╗██║     ██║   ██║██████╔╝███████║██║     ███████╗█████╗     ██║      ██║   ██║██╔██╗ ██║██║  ███╗███████╗   ██║     ██████╔╝██████╔╝
██║   ██║██║     ██║   ██║██╔══██╗██╔══██║██║     ╚════██║██╔══╝     ██║      ██║   ██║██║╚██╗██║██║   ██║╚════██║   ██║     ██╔═══╝ ██╔═══╝ 
╚██████╔╝███████╗╚██████╔╝██████╔╝██║  ██║███████╗███████║███████╗   ██║      ██║   ██║██║ ╚████║╚██████╔╝███████║██╗╚██████╗██║     ██║     
 ╚═════╝ ╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝   ╚═╝      ╚═╝   ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

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

#include <globalSettings.h>

std::vector<std::pair<GlobalSettings::SettingType, std::function<void()>>> GlobalSettings::settingChangeCallbacks = {};
std::mutex GlobalSettings::settingChangeMutex;
nlohmann::json GlobalSettings::settings = nlohmann::json();
std::unordered_map <GlobalSettings::SettingType, std::string> GlobalSettings::settingTypeStringMap = {
        {SettingType::LOG_VERBOSITY, "logVerbosity"},
        {SettingType::SELECTED_FONT_PATH, "selectedFontPath"},
        {SettingType::LOG_LEVEL_PRINT, "logLevelP"},
        {SettingType::LOG_LEVEL_FILE, "logLevelF"},
        {SettingType::DEVELOPER_MODE_ENABLED, "developerModeEnabled"},
        {SettingType::CPU_AND_MEMORY_DISPLAY_ENABLED, "cpuAndMemoryDisplayEnabled"},
        {SettingType::SSH_ENABLED, "sshEnabled"},
        {SettingType::WIFI_ENABLED, "wifiEnabled"},
        {SettingType::WIFI_SSID, "wifiSSID"},
        {SettingType::WIFI_PASSWORD, "wifiPassword"},
        {SettingType::BT_ENABLED, "btEnabled"},
        {SettingType::STATUS_IP_ADDRESS, "statusIPAddress"},
        {SettingType::STATUS_WIFI_MAC_ADDRESS, "statusWifiMACAddress"},
        {SettingType::STATUS_BT_MAC_ADDRESS, "statusBTMACAddress"},
        {SettingType::STATUS_FCC_ID, "statusFCCID"},
        {SettingType::IDLE_ANIMATION_ENABLED, "idleAnimationEnabled"},
        {SettingType::IDLE_ANIMATION, "idleAnimation"},
        {SettingType::SCREEN_BRIGHTNESS, "screenBrightness"},
        {SettingType::SCREEN_AUTO_OFF, "screenAutoOff"},
        {SettingType::SCREEN_AUTO_OFF_TIME, "screenAutoOffTime"},
        {SettingType::NOTIFICATIONS_FROM_NETWORK_ENABLED, "notificationsFromNetworkEnabled"},
        {SettingType::SYSTEM_VOLUME, "systemVolume"},
        {SettingType::NOTIFICATION_SOUND, "notificationSound"},
        {SettingType::ALARM_SOUND, "alarmSound"},
        {SettingType::VOICE_COMMAND_SOUND, "voiceCommandSound"},
        {SettingType::NFC_ENABLED, "nfcEnabled"},
        {SettingType::MICROPHONE_ENABLED, "microphoneEnabled"},
        {SettingType::PRESENCE_DETECTION_ENABLED, "presenceDetectionEnabled"},
        {SettingType::EMOTION_CURIOSITY, "emotionCuriosity"},
        {SettingType::EMOTION_PLAYFULNESS, "emotionPlayfulness"},
        {SettingType::EMOTION_EMPATHY, "emotionEmpathy"},
        {SettingType::EMOTION_ASSERTIVENESS, "emotionAssertiveness"},
        {SettingType::EMOTION_ATTENTIVENESS, "emotionAttentiveness"},
        {SettingType::EMOTION_CAUTION, "emotionCaution"},
        {SettingType::EMOTION_ANNOYANCE, "emotionAnnoyance"},
        {SettingType::REMOTE_INTENT_RECOGNITION_ENABLED, "remoteIntentRecognitionEnabled"},
        {SettingType::REMOTE_TRANSCRIPTION_ENABLED, "remoteTranscriptionEnabled"}
    };
std::unordered_map <std::string, GlobalSettings::SettingType> GlobalSettings::stringSettingTypeMap = {
        {"logVerbosity", SettingType::LOG_VERBOSITY},
        {"selectedFontPath", SettingType::SELECTED_FONT_PATH},
        {"logLevelP", SettingType::LOG_LEVEL_PRINT},
        {"logLevelF", SettingType::LOG_LEVEL_FILE},
        {"developerModeEnabled", SettingType::DEVELOPER_MODE_ENABLED},
        {"cpuAndMemoryDisplayEnabled", SettingType::CPU_AND_MEMORY_DISPLAY_ENABLED},
        {"sshEnabled", SettingType::SSH_ENABLED},
        {"wifiEnabled", SettingType::WIFI_ENABLED},
        {"wifiSSID", SettingType::WIFI_SSID},
        {"wifiPassword", SettingType::WIFI_PASSWORD},
        {"btEnabled", SettingType::BT_ENABLED},
        {"statusIPAddress", SettingType::STATUS_IP_ADDRESS},
        {"statusWifiMACAddress", SettingType::STATUS_WIFI_MAC_ADDRESS},
        {"statusBTMACAddress", SettingType::STATUS_BT_MAC_ADDRESS},
        {"statusFCCID", SettingType::STATUS_FCC_ID},
        {"idleAnimationEnabled", SettingType::IDLE_ANIMATION_ENABLED},
        {"idleAnimation", SettingType::IDLE_ANIMATION},
        {"screenBrightness", SettingType::SCREEN_BRIGHTNESS},
        {"screenAutoOff", SettingType::SCREEN_AUTO_OFF},
        {"screenAutoOffTime", SettingType::SCREEN_AUTO_OFF_TIME},
        {"notificationsFromNetworkEnabled", SettingType::NOTIFICATIONS_FROM_NETWORK_ENABLED},
        {"systemVolume", SettingType::SYSTEM_VOLUME},
        {"notificationSound", SettingType::NOTIFICATION_SOUND},
        {"alarmSound", SettingType::ALARM_SOUND},
        {"voiceCommandSound", SettingType::VOICE_COMMAND_SOUND},
        {"nfcEnabled", SettingType::NFC_ENABLED},
        {"microphoneEnabled", SettingType::MICROPHONE_ENABLED},
        {"presenceDetectionEnabled", SettingType::PRESENCE_DETECTION_ENABLED},
        {"emotionCuriosity", SettingType::EMOTION_CURIOSITY},
        {"emotionPlayfulness", SettingType::EMOTION_PLAYFULNESS},
        {"emotionEmpathy", SettingType::EMOTION_EMPATHY},
        {"emotionAssertiveness", SettingType::EMOTION_ASSERTIVENESS},
        {"emotionAttentiveness", SettingType::EMOTION_ATTENTIVENESS},
        {"emotionCaution", SettingType::EMOTION_CAUTION},
        {"emotionAnnoyance", SettingType::EMOTION_ANNOYANCE},
        {"remoteIntentRecognitionEnabled", SettingType::REMOTE_INTENT_RECOGNITION_ENABLED},
        {"remoteTranscriptionEnabled", SettingType::REMOTE_TRANSCRIPTION_ENABLED}
    };