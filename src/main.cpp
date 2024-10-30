#define SW_VERSION "0.1.0"

#include "main.h"


int main(int argc, char* argv[])
{
    std::cout << "Starting..." << std::endl;
#ifdef __linux__
    // TODO: rather than set this environment variable in this application, set it in the manager application.
    if (setenv("DISPLAY", ":0", 1) != 0) {
        std::cout << "Error setting DISPLAY=:0 environment variable. Exiting." << std::endl;
        return 1;
    }
#endif
    /////////////////////////////////////////////////////////////////
    // Argument parsing
    /////////////////////////////////////////////////////////////////
    std::string versionInfo = "Companion, TheCube - CORE ver: ";
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
            if (Logger::LogVerbosity(std::stoi(value)) >= Logger::LogVerbosity::LOGVERBOSITYCOUNT)
                throw std::runtime_error("Invalid log level.");
            customLogVerbosity = true;
            return std::stoi(value);
        });
    argumentParser.add_argument("-L", "--LogLevelP")
        .help("Set log level for printing to console (0-6)")
        .default_value(3)
        .action([&](const std::string& value) {
            if (Logger::LogLevel(std::stoi(value)) >= Logger::LogLevel::LOGGER_LOGLEVELCOUNT)
                throw std::runtime_error("Invalid log level.");
            customLogLevelP = true;
            return std::stoi(value);
        });
    argumentParser.add_argument("-f", "--LogLevelF")
        .help("Set log level for writing to file (0-6)")
        .default_value(3)
        .action([&](const std::string& value) {
            if (Logger::LogLevel(std::stoi(value)) >= Logger::LogLevel::LOGGER_LOGLEVELCOUNT)
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
    auto settingsLoader = new SettingsLoader(&settings); // This could probably just be a function.
    settingsLoader->loadSettings();
    delete settingsLoader;
    if (argumentParser["--print"] == true) {
        std::cout << "\n\n" + settings.toString() << std::endl;
        exit(0);
    }
    if (customLogVerbosity)
        settings.setSetting(GlobalSettings::SettingType::LOG_VERBOSITY, logVerbosity);
    logger->setVerbosity(settings.getSettingOfType<Logger::LogVerbosity>(GlobalSettings::SettingType::LOG_VERBOSITY));
    if (customLogLevelP)
        settings.setSetting(GlobalSettings::SettingType::LOG_LEVEL_PRINT, logLevelPrint);
    if (customLogLevelF)
        settings.setSetting(GlobalSettings::SettingType::LOG_LEVEL_FILE, logLevelFile);
    logger->setLogLevel(settings.getSettingOfType<Logger::LogLevel>(GlobalSettings::SettingType::LOG_LEVEL_PRINT), settings.getSettingOfType<Logger::LogLevel>(GlobalSettings::SettingType::LOG_LEVEL_FILE));
    if (supportsExtendedColors()) {
        CubeLog::info("Extended colors supported.");
    } else if (supportsBasicColors()) {
        CubeLog::info("Basic colors supported.");
    } else {
        CubeLog::info("No colors supported.");
    }
    CubeLog::info("Logger initialized.");
    CubeLog::info("Settings loaded.");
    CubeLog::info("Loading GUI...");
    auto gui = std::make_shared<GUI>();
    /////////////////////////////////////////////////////////////////
    // CPU and memory monitor thread
    /////////////////////////////////////////////////////////////////
#ifndef PRODUCTION_BUILD
    std::jthread cpuAndMemoryThread([](std::stop_token st) {
        unsigned long loopCount = 0;
        while (!st.stop_requested()) {
            if (loopCount++ % 100 == 0) monitorMemoryAndCPU();
            genericSleep(100);
        }
    });
#endif
    /////////////////////////////////////////////////////////////////
    // Logger test
    /////////////////////////////////////////////////////////////////
    CubeLog::info("Test info message.");
    CubeLog::debug("Test debug message.");
    CubeLog::debugSilly("Test debug silly message.");
    CubeLog::error("Test error message.");
    CubeLog::warning("Test warning message.");
    CubeLog::critical("Test critical message.");
    CubeLog::moreInfo("Test more info message.");
    CubeLog::fatal("Test fatal message.");

    CubeLog::debugSilly("MajorVersion: " + std::to_string(MAJOR_VERSION));
    CubeLog::debugSilly("MinorVersion: " + std::to_string(MINOR_VERSION));
    CubeLog::debugSilly("PatchVersion: " + std::to_string(PATCH_VERSION));
    CubeLog::debugSilly("BuildVersion: " + std::to_string(BUILD_NUMBER));
    /////////////////////////////////////////////////////////////////
    // Main loop
    /////////////////////////////////////////////////////////////////
    {
        auto audioManager = std::make_shared<AudioManager>();
        auto db_manager = std::make_shared<CubeDatabaseManager>();
        auto blobs = std::make_shared<BlobsManager>(db_manager, "data/blobs.db");
        auto cubeDB = std::make_shared<CubeDB>(db_manager, blobs);
        auto wifiManager = std::make_shared<WifiManager>();
        auto btManager = std::make_shared<BTManager>();
        // Testing /////////////////////////////////////////////////
        long blobID = CubeDB::getBlobsManager()->addBlob("client_blobs", "test blob", "1");
        CubeLog::info("Blob ID: " + std::to_string(blobID));
        bool allInsertionsSuccess = true;
        // TODO: All the base apps should be inserted into the database and/or verified in the database here.
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
        // end testing //////////////////////////////////////////////
        AppsManager appsManager;
        auto api = std::make_shared<API>();
        auto auth = std::make_shared<CubeAuth>();
        API_Builder api_builder(api);
        api_builder.addInterface(gui);
        api_builder.addInterface(cubeDB);
        api_builder.addInterface(logger);
        api_builder.addInterface(audioManager);
        api_builder.addInterface(auth);
        // api_builder.addInterface(wifiManager);
        api_builder.addInterface(btManager);
        api_builder.start();
        CubeLog::info("Entering main loop...");
        while (true) {
            genericSleep(100);
            std::string input;
            std::cin >> input;
            if (input == "exit" || input == "quit" || input == "q" || input == "e" || input == "x") {
                break;
            } else if (input == "sound") {
                audioManager->toggleSound();
            }
        }
        CubeLog::info("Exited main loop...");
    }
    return 0;
    // TODO: any other place where main() might return, change the return value to something meaningful. this way, when
    // we get around to launching this with the node.js manager app, we can use the return value to determine if the
    // application should be restarted or not.
}

#ifdef _WIN32

bool supportsBasicColors()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    return (dwMode & ENABLE_PROCESSED_OUTPUT) && (dwMode & ENABLE_WRAP_AT_EOL_OUTPUT);
}

bool supportsExtendedColors()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    if (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
        return true;
    }

    // Try to enable the flag
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return false;
    }

    return true;
}

#elif __linux__


int getTermColors()
{
    const char* term = std::getenv("TERM");
    if (!term) {
        return false;
    }

    std::string termStr(term);
    if (termStr == "dumb") {
        return false;
    }

    FILE* pipe = popen("tput colors", "r");
    if (!pipe) {
        return false;
    }

    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);

    return std::stoi(result);
}

bool supportsBasicColors()
{
    return getTermColors() >= 8;
}

bool supportsExtendedColors()
{
    return getTermColors() >= 256;
}

#endif