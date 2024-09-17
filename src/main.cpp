#define SW_VERSION "0.1.0"

#include "main.h"

// Two-channel sawtooth wave generator.
int saw(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData)
{
    unsigned int i, j;
    double* buffer = (double*)outputBuffer;
    double* lastValues = (double*)userData;

    if (status)
        std::cout << "Stream underflow detected!" << std::endl;

    // Write interleaved audio data.
    for (i = 0; i < nBufferFrames; i++) {
        for (j = 0; j < 2; j++) {
            if (lastValues[2] == 0)
                *buffer++ = 0.0;
            else
                *buffer++ = lastValues[j];

            lastValues[j] += 0.005 * (j + 1 + (j * 0.1));
            if (lastValues[j] >= 1.0)
                lastValues[j] -= 2.0;
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    std::cout << "Starting..." << std::endl;
#ifdef __linux___
    // if (setenv("DISPLAY", ":1", 1) != 0) {
    //     std::cout << "Error setting DISPLAY=:0 environment variable. Exiting." << std::endl;
    //     return 1;
    // }
#endif
    /////////////////////////////////////////////////////////////////
    // Argument parsing
    /////////////////////////////////////////////////////////////////
    std::string versionInfo = "Companion, TheCube - CORE ver.";
    versionInfo += SW_VERSION;
    versionInfo += "\n";
    versionInfo += "Built on: " + std::string(__DATE__) + " " + std::string(__TIME__) + "\n";
    versionInfo += "Built on: ";
#ifdef __linux__
    versionInfo += "Linux";
#endif
#ifdef _WIN32
    versionInfo += "Windows";
#endif
    versionInfo += "\nCompiler: ";
#ifdef __clang__
    versionInfo += "Clang";
#endif
#ifdef __GNUC__
    versionInfo += "GCC";
#endif
#ifdef _MSC_VER
    versionInfo += "MSVC";
#endif
    versionInfo += "\nC++ Standard: ";
#ifdef __cplusplus
#ifdef _MSVC_LANG
    if (_MSVC_LANG == 202300L)
        versionInfo += "C++23";
    else if (_MSVC_LANG == 202100L)
        versionInfo += "C++21";
    else if (_MSVC_LANG == 202002L)
        versionInfo += "C++20";
    else if (_MSVC_LANG == 201703L)
        versionInfo += "C++17";
    else if (_MSVC_LANG == 201402L)
        versionInfo += "C++14";
    else if (_MSVC_LANG == 201103L)
        versionInfo += "C++11";
    else
        versionInfo += std::to_string(_MSVC_LANG);
#else
    if (__cplusplus == 202300L)
        versionInfo += "C++23";
    else if (__cplusplus == 202100L)
        versionInfo += "C++21";
    else if (__cplusplus == 202002L)
        versionInfo += "C++20";
    else if (__cplusplus == 201703L)
        versionInfo += "C++17";
    else if (__cplusplus == 201402L)
        versionInfo += "C++14";
    else if (__cplusplus == 201103L)
        versionInfo += "C++11";
    else
        versionInfo += std::to_string(__cplusplus);
#endif
#endif
    versionInfo += "\nAuthor: Andrew McDaniel\nCopyright: 2024\n";
    bool customLogVerbosity = false;
    bool customLogLevelP = false;
    bool customLogLevelF = false;
    argparse::ArgumentParser argumentParser("Companion, TheCube - CORE", versionInfo);
    argumentParser.add_argument("-l", "--logVerbosity")
        .help("Set log verbosity (0-6)")
        .default_value(3)
        .action([&](const std::string& value) {
            if (LogVerbosity(std::stoi(value)) >= LogVerbosity::LOGVERBOSITYCOUNT)
                throw std::runtime_error("Invalid log level.");
            customLogVerbosity = true;
            return std::stoi(value);
        });
    argumentParser.add_argument("-L", "--LogLevelP")
        .help("Set log level for printing to console (0-6)")
        .default_value(3)
        .action([&](const std::string& value) {
            if (LogLevel(std::stoi(value)) >= LogLevel::LOGGER_LOGLEVELCOUNT)
                throw std::runtime_error("Invalid log level.");
            customLogLevelP = true;
            return std::stoi(value);
        });
    argumentParser.add_argument("-f", "--LogLevelF")
        .help("Set log level for writing to file (0-6)")
        .default_value(3)
        .action([&](const std::string& value) {
            if (LogLevel(std::stoi(value)) >= LogLevel::LOGGER_LOGLEVELCOUNT)
                throw std::runtime_error("Invalid log level.");
            customLogLevelF = true;
            return std::stoi(value);
        });
    argumentParser.add_argument("-p", "--print")
        .help("Print settings to console and exit.")
        .default_value(false)
        .implicit_value(true);
    try {
        argumentParser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << argumentParser;
        exit(0);
    }
    std::cout << versionInfo << std::endl;
    auto logVerbosity = argumentParser.get<int>("--logVerbosity");
    auto logLevelPrint = argumentParser.get<int>("--LogLevelP");
    auto logLevelFile = argumentParser.get<int>("--LogLevelF");    
    /////////////////////////////////////////////////////////////////
    // Settings and Logger setup
    /////////////////////////////////////////////////////////////////
    GlobalSettings settings;
    auto logger = std::make_shared<CubeLog>();
    auto settingsLoader = new SettingsLoader(&settings);
    settingsLoader->loadSettings();
    delete settingsLoader;
    if (argumentParser["--print"] == true) {
        std::cout << "\n\n" + settings.toString() << std::endl;
        exit(0);
    }
    if (customLogVerbosity)
        settings.logVerbosity = LogVerbosity(logVerbosity);
    logger->setVerbosity(settings.logVerbosity);
    if (customLogLevelP)
        settings.setSetting("LogLevelP", logLevelPrint);
    if (customLogLevelF)
        settings.setSetting("LogLevelF", logLevelFile);
    logger->setLogLevel(settings.logLevelPrint, settings.logLevelFile);
    if (supportsExtendedColors()) {
        CubeLog::info("Extended colors supported.");
    } else if (supportsBasicColors()) {
        CubeLog::info("Basic colors supported.");
    } else {
        CubeLog::info("No colors supported.");
    }
    CubeLog::info("Logger initialized.");
    CubeLog::info("Settings loaded.");
    /////////////////////////////////////////////////////////////////
    CubeLog::info("Loading GUI...");
    auto gui = std::make_shared<GUI>();

    /////////////////////////////////////////////////////////////////
    // RtAudio setup
    /////////////////////////////////////////////////////////////////
#ifdef __linux__
    RtAudio::Api api = RtAudio::RtAudio::LINUX_ALSA;
#elif _WIN32
    RtAudio::Api api = RtAudio::RtAudio::WINDOWS_DS;
#endif
    RtAudio dac(api);
    std::vector<unsigned int> deviceIds = dac.getDeviceIds();
    std::vector<std::string> deviceNames = dac.getDeviceNames();
    if (deviceIds.size() < 1) {
        std::cout << "\nNo audio devices found! Exiting.\n";
        exit(0);
    }
    CubeLog::info("Audio devices found: " + std::to_string(deviceIds.size()));
    for (auto device : deviceNames) {
        CubeLog::info("Device: " + device);
    }

    RtAudio::StreamParameters parameters;
    CubeLog::info("Setting up audio stream with default audio device.");
    parameters.deviceId = dac.getDefaultOutputDevice();
    // find the device name for the audio device id
    std::string deviceName = dac.getDeviceInfo(parameters.deviceId).name;
    CubeLog::info("Using audio device: " + deviceName);
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 256; // 256 sample frames
    double data[3] = { 0, 0, 0 };

    if (dac.openStream(&parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &saw, (void*)&data)) {
        CubeLog::error(dac.getErrorText() + " Exiting.");
        exit(0); // problem with device settings
    }

    // Stream is open ... now start it.
    if (dac.startStream()) {
        std::cout << dac.getErrorText() << std::endl;
        CubeLog::error(dac.getErrorText());
    }
    CubeLog::info("Audio stream started.");

    std::jthread cpuAndMemoryThread([](std::stop_token st) {
        unsigned long loopCount = 0;
        while (!st.stop_requested()) {
            if (loopCount++ % 100 == 0) monitorMemoryAndCPU();
            genericSleep(100);
        }
    });
    /////////////////////////////////////////////////////////////////
    // Logger test
    /////////////////////////////////////////////////////////////////
    CubeLog::info("Test info message.");
    CubeLog::debug("Test debug message.");
    CubeLog::error("Test error message.");
    CubeLog::warning("Test warning message.");
    CubeLog::critical("Test critical message.");
    /////////////////////////////////////////////////////////////////
    // Main loop
    /////////////////////////////////////////////////////////////////
    {
        auto db_cube = std::make_shared<CubeDatabaseManager>();
        auto blobs = std::make_shared<BlobsManager>(db_cube, "data/blobs.db");
        auto cubeDB = std::make_shared<CubeDB>(db_cube, blobs);
        long blobID = CubeDB::getBlobsManager()->addBlob("client_blobs", "test blob", "1");
        CubeLog::info("Blob ID: " + std::to_string(blobID));
        bool allInsertionsSuccess = true;
        // TODO: All the base apps should be inserted into the database and /r verified in the database here.
        // allInsertionsSuccess &=  (-1 < CubeDB::getDBManager()->getDatabase("apps")->insertData("apps", { "app_id", "app_name", "role", "exec_path", "exec_args", "app_source", "update_path", "update_last_check", "update_last_update", "update_last_fail", "update_last_fail_reason" }, { "1", "CMD", "native", "apps\\customcmd", "", "test source", "test update path", "test last check", "test last update", "test last fail", "test last fail reason" })); // test insert
        #ifdef __linux__
        allInsertionsSuccess &= (-1 < CubeDB::getDBManager()->getDatabase("apps")->insertData("apps", { "app_id", "app_name", "role", "exec_path", "exec_args", "app_source", "update_path", "update_last_check", "update_last_update", "update_last_fail", "update_last_fail_reason" }, { "2", "ConsoleApp1", "native", "apps\\consoleApp1\\consoleApp1", "arg1 arg2 arg3 arg4", "test source", "test update path", "test last check", "test last update", "test last fail", "test last fail reason" })); // test insert
        allInsertionsSuccess &= (-1 < CubeDB::getDBManager()->getDatabase("apps")->insertData("apps", { "app_id", "app_name", "role", "exec_path", "exec_args", "app_source", "update_path", "update_last_check", "update_last_update", "update_last_fail", "update_last_fail_reason" }, { "3", "ConsoleApp2", "native", "apps\\consoleApp1\\consoleApp1", "arg5 arg6 arg7 arg8", "test source", "test update path", "test last check", "test last update", "test last fail", "test last fail reason" })); // test insert
        #endif
        #ifdef _WIN32
        allInsertionsSuccess &= (-1 < CubeDB::getDBManager()->getDatabase("apps")->insertData("apps", { "app_id", "app_name", "role", "exec_path", "exec_args", "app_source", "update_path", "update_last_check", "update_last_update", "update_last_fail", "update_last_fail_reason" }, { "2", "ConsoleApp1", "native", "apps/consoleApp1.exe", "arg1 arg2 arg3 arg4", "test source", "test update path", "test last check", "test last update", "test last fail", "test last fail reason" })); // test insert
        allInsertionsSuccess &= (-1 < CubeDB::getDBManager()->getDatabase("apps")->insertData("apps", { "app_id", "app_name", "role", "exec_path", "exec_args", "app_source", "update_path", "update_last_check", "update_last_update", "update_last_fail", "update_last_fail_reason" }, { "3", "ConsoleApp2", "native", "apps/consoleApp1.exe", "arg5 arg6 arg7 arg8", "test source", "test update path", "test last check", "test last update", "test last fail", "test last fail reason" })); // test insert
        #endif
        if (!allInsertionsSuccess)
            CubeLog::warning("Failed to insert data into database. Last error: " + CubeDB::getDBManager()->getDatabase("apps")->getLastError());
        AppsManager appsManager;
        auto api = std::make_shared<API>();
        API_Builder api_builder(api);
        api_builder.addInterface(gui);
        api_builder.addInterface(cubeDB);
        api_builder.addInterface(logger);
        api_builder.start();
        CubeLog::info("Entering main loop...");
        while (true) {
            genericSleep(100);
            // get cin and check if it's "exit", "quit", "q", or "e", then break, or if it's "sound" toggle the sound
            std::string input;
            std::cin >> input;
            if (input == "exit" || input == "quit" || input == "q" || input == "e") {
                break;
            } else if (input == "sound") {
                if (data[2] == 0) data[2] = 1;
                else data[2] = 0;
            }
        }
        CubeLog::info("Exited main loop...");
        std::cout << "Exiting..." << std::endl;
        // sineWaveSound.stop();
    }
    
    dac.stopStream();
    if (dac.isStreamOpen())
        dac.closeStream();
    return 0;
}
