/*
███╗   ███╗ █████╗ ██╗███╗   ██╗    ██████╗██████╗ ██████╗
████╗ ████║██╔══██╗██║████╗  ██║   ██╔════╝██╔══██╗██╔══██╗
██╔████╔██║███████║██║██╔██╗ ██║   ██║     ██████╔╝██████╔╝
██║╚██╔╝██║██╔══██║██║██║╚██╗██║   ██║     ██╔═══╝ ██╔═══╝
██║ ╚═╝ ██║██║  ██║██║██║ ╚████║██╗╚██████╗██║     ██║
╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

/*
 ██████╗ ██████╗ ███╗   ███╗██████╗  █████╗ ███╗   ██╗██╗ ██████╗ ███╗   ██╗       ████████╗██╗  ██╗███████╗ ██████╗██╗   ██╗██████╗ ███████╗
██╔════╝██╔═══██╗████╗ ████║██╔══██╗██╔══██╗████╗  ██║██║██╔═══██╗████╗  ██║       ╚══██╔══╝██║  ██║██╔════╝██╔════╝██║   ██║██╔══██╗██╔════╝
██║     ██║   ██║██╔████╔██║██████╔╝███████║██╔██╗ ██║██║██║   ██║██╔██╗ ██║          ██║   ███████║█████╗  ██║     ██║   ██║██████╔╝█████╗
██║     ██║   ██║██║╚██╔╝██║██╔═══╝ ██╔══██║██║╚██╗██║██║██║   ██║██║╚██╗██║          ██║   ██╔══██║██╔══╝  ██║     ██║   ██║██╔══██╗██╔══╝
╚██████╗╚██████╔╝██║ ╚═╝ ██║██║     ██║  ██║██║ ╚████║██║╚██████╔╝██║ ╚████║▄█╗       ██║   ██║  ██║███████╗╚██████╗╚██████╔╝██████╔╝███████╗
 ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝       ╚═╝   ╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝
                                                     ██████╗ ██████╗ ██████╗ ███████╗
                                                    ██╔════╝██╔═══██╗██╔══██╗██╔════╝
                                                    ██║     ██║   ██║██████╔╝█████╗
                                                    ██║     ██║   ██║██╔══██╗██╔══╝
                                                    ╚██████╗╚██████╔╝██║  ██║███████╗
                                                     ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
*/
#define SW_VERSION "0.1.0"
#ifndef LOGGER_H
#include <logger.h>
#endif
#include "build_number.h"
#include "main.h"

bool breakMain = false;

int main(int argc, char* argv[])
{
    // std::signal(SIGINT, signalHandler);
    // std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGILL, signalHandler);
    std::signal(SIGFPE, signalHandler);
#ifdef __linux__
    std::signal(SIGKILL, signalHandler);
    std::signal(SIGQUIT, signalHandler);
    std::signal(SIGBUS, signalHandler);
    std::signal(SIGSYS, signalHandler);
    std::signal(SIGPIPE, signalHandler);
    std::signal(SIGALRM, signalHandler);
    std::signal(SIGHUP, signalHandler);
#endif
    std::cout << "Starting..." << std::endl;
#ifdef __linux__
    // TODO: rather than set this environment variable in this application, set it in the manager application. The manager app
    // should also verify if a symbolic link is needed and create it if necessary.
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
    logger->setLogLevel(
        settings.getSettingOfType<Logger::LogLevel>(GlobalSettings::SettingType::LOG_LEVEL_PRINT),
        settings.getSettingOfType<Logger::LogLevel>(GlobalSettings::SettingType::LOG_LEVEL_FILE));
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
    // CPU and memory monitor thread
    /////////////////////////////////////////////////////////////////
#ifndef PRODUCTION_BUILD
    std::jthread cpuAndMemoryThread([](std::stop_token st) {
        unsigned long loopCount = 0;
        while (!st.stop_requested()) {
            if (loopCount++ % 100 == 0)
                monitorMemoryAndCPU();
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
        CubeLog::info("Loading GUI...");
        auto gui = std::make_shared<GUI>();
        auto audioManager = std::make_shared<AudioManager>();
        auto db_manager = std::make_shared<CubeDatabaseManager>();
        auto blobs = std::make_shared<BlobsManager>(db_manager, "data/blobs.db");
        auto cubeDB = std::make_shared<CubeDB>(db_manager, blobs);
        auto wifiManager = std::make_shared<WifiManager>();
        // auto btManager = std::make_shared<BTManager>();
        // Testing /////////////////////////////////////////////////
        long blobID = CubeDB::getBlobsManager()->addBlob("client_blobs", "test blob", "1");
        CubeLog::info("Blob ID: " + std::to_string(blobID));
        bool allInsertionsSuccess = true;
        long dbInsertReturnVal = -1;
        // create string for apps db delete where clause so that it catches all apps with app_id 0 to 9
        std::string deleteWhereClause = "app_id IN (";
        for (int i = 0; i < 10; ++i) {
            deleteWhereClause += std::to_string(i);
            if (i < 9)
                deleteWhereClause += ", ";
        }
        deleteWhereClause += ")";
        // delete all apps with app_id 0 to 9
        CubeLog::info("Deleting all apps with app_id 0 to 9 from apps database.");
        CubeDB::getDBManager()->getDatabase("apps")->deleteData(DB_NS::TableNames::APPS, deleteWhereClause);
        // TODO: All the base apps should be inserted into the database and/or verified in the database here.
        dbInsertReturnVal = CubeDB::getDBManager()->getDatabase("apps")->insertData(
            DB_NS::TableNames::APPS,
            { { "app_id", "2" },
                { "app_name", "ConsoleApp1" },
                { "role", DB_NS::Roles::NATIVE_APP },
#ifdef _WIN32
                { "exec_path", "apps/consoleApp1.exe" },
#else
                { "exec_path", "apps/consoleApp1/consoleApp1" },
#endif
                { "exec_args", "arg1 arg2 arg3 arg4" },
                { "app_source", "test source" },
                { "update_path", "test update path" },
                { "update_last_check", "test last check" },
                { "update_last_update", "test last update" },
                { "update_last_fail", "test last fail" },
                { "update_last_fail_reason", "test last fail reason" } }); // test insert
        allInsertionsSuccess &= (-1 < dbInsertReturnVal);
        dbInsertReturnVal = CubeDB::getDBManager()->getDatabase("apps")->insertData(
            DB_NS::TableNames::APPS,
            { { "app_id", "3" },
                { "app_name", "ConsoleApp2" },
                { "role", DB_NS::Roles::NATIVE_APP },
#ifdef _WIN32
                { "exec_path", "apps/consoleApp1.exe" },
#else
                { "exec_path", "apps/consoleApp1/consoleApp1" },
#endif
                { "exec_args", "arg5 arg6 arg7 arg8" },
                { "app_source", "test source" },
                { "update_path", "test update path" },
                { "update_last_check", "test last check" },
                { "update_last_update", "test last update" },
                { "update_last_fail", "test last fail" },
                { "update_last_fail_reason", "test last fail reason" } }); // test insert
        allInsertionsSuccess &= (-1 < dbInsertReturnVal);
        dbInsertReturnVal = CubeDB::getDBManager()->getDatabase("apps")->insertData(
            DB_NS::TableNames::APPS,
            { { "app_id", "5" },
                { "app_name", "openwakeword" },
                { "role", DB_NS::Roles::NATIVE_APP },
#ifdef _WIN32
                { "exec_path", "apps/consoleApp1.exe" },
#else
                { "exec_path", "apps/openwakeword/bin/python3" },
#endif
                { "exec_args", "apps/openwakeword/openww.py --model_path apps/openwakeword/hey_cube.onnx,apps/openwakeword/hey_cuba.onnx,apps/openwakeword/hey_cube2.onnx,apps/openwakeword/hey_cue.onnx --inference_framework onnx" },
                { "app_source", "test source" },
                { "update_path", "test update path" },
                { "update_last_check", "test last check" },
                { "update_last_update", "test last update" },
                { "update_last_fail", "test last fail" },
                { "update_last_fail_reason", "test last fail reason" } }); // test insert
        allInsertionsSuccess &= (-1 < dbInsertReturnVal);
        // TODO: add the insert for the openwakeword python script here. This will be a native app and will use the python executable in the openwakeword/venv/bin(Linux) or openwakeword/Scripts(Windows) directory.
        if (!allInsertionsSuccess)
            CubeLog::warning("Failed to insert data into database. Last error: " + CubeDB::getDBManager()->getDatabase("apps")->getLastError());
        // end testing //////////////////////////////////////////////
        AppsManager appsManager;
        auto api = std::make_shared<API>();
        auto auth = std::make_shared<CubeAuth>();
        auto peripherals = std::make_shared<PeripheralManager>();
        auto decisions = std::make_shared<DecisionEngine::DecisionEngineMain>();
        
            API_Builder api_builder(api);
            gui->registerInterface();
            cubeDB->registerInterface();
            logger->registerInterface();
            audioManager->registerInterface();
            auth->registerInterface();
            // btManager->registerInterface();
            api_builder.start();
        
        CubeLog::info("Entering main loop...");
        std::chrono::milliseconds aSecond(1000);
        while (!breakMain) {
            std::string input;
            if (!(std::cin >> input)) {
                breakMain = true;
                break;
            }
            if (input == "exit" || input == "quit" || input == "q" || input == "e" || input == "x") {
                breakMain = true;
                break;
            } else if (input == "sound") {
                audioManager->toggleSound();
            }
        }
        CubeLog::info("Exited main loop...");
        CubeLog::info("CubeLog reference count: " + std::to_string(logger.use_count()));
        cpuAndMemoryThread.request_stop();
        CubeLog::info("Stopping GUI...");
        gui->stop();
    }
    std::cout << "Exiting..." << std::endl;
    return 0;
    // TODO: any other place where main() might return, change the return value to something meaningful. this way, when
    // we get around to launching this with the node.js manager app, we can use the return value to determine if the
    // application should be restarted or not.
}

// TODO: this probably needs a mutex for breakMain
void signalHandler(int signal)
{
    switch (signal) {
    case SIGINT:
        CubeLog::info("Caught SIGINT signal.");
        // printStackTrace();
        breakMain = true;
        break;
    case SIGTERM:
        CubeLog::info("Caught SIGTERM signal.");
        // printStackTrace();
        breakMain = true;
        break;
    case SIGABRT:
        CubeLog::info("Caught SIGABRT signal.");
        printStackTrace();
        breakMain = true;
        break;
    case SIGSEGV:
        CubeLog::info("Caught SIGSEGV signal.");
        // get the stack trace
        printStackTrace();
        breakMain = true;
        exit(1);
        break;
    case SIGILL:
        CubeLog::info("Caught SIGILL signal.");
        printStackTrace();
        breakMain = true;
        break;
    case SIGFPE:
        CubeLog::info("Caught SIGFPE signal.");
        printStackTrace();
        breakMain = true;
        break;
#ifdef __linux__
    case SIGKILL:
        CubeLog::info("Caught SIGKILL signal.");
        // Note: SIGKILL cannot be caught or ignored, so this will not actually execute.
        breakMain = true;
        break;
    case SIGQUIT:
        CubeLog::info("Caught SIGQUIT signal.");
        breakMain = true;
        break;
    case SIGBUS:
        CubeLog::info("Caught SIGBUS signal.");
        breakMain = true;
        break;
    case SIGSYS:
        CubeLog::info("Caught SIGSYS signal.");
        breakMain = true;
        break;
    case SIGPIPE:
        CubeLog::info("Caught SIGPIPE signal.");
        breakMain = true;
        break;
    case SIGALRM:
        CubeLog::info("Caught SIGALRM signal.");
        breakMain = true;
        break;
    case SIGHUP:
        CubeLog::info("Caught SIGHUP signal.");
        printStackTrace();
        breakMain = true;
        break;
#endif
    default:
        CubeLog::info("Caught unknown signal.");
        breakMain = true;
        break;
    }
}

// Function to print stack trace
void printStackTrace()
{
#ifdef __linux__
    const int maxFrames = 100;
    void* frames[maxFrames];
    int numFrames = backtrace(frames, maxFrames);
    char** symbols = backtrace_symbols(frames, numFrames);

    if (symbols == nullptr) {
        std::cerr << "Failed to generate stack trace.\n";
        return;
    }

    std::cerr << "Stack trace:\n";
    bool debugSymbolsIncluded = false;

    for (int i = 0; i < numFrames; ++i) {
        char *mangledName = nullptr, *offsetBegin = nullptr, *offsetEnd = nullptr;

        for (char* p = symbols[i]; *p; ++p) {
            if (*p == '(')
                mangledName = p;
            else if (*p == '+')
                offsetBegin = p;
            else if (*p == ')' && offsetBegin) {
                offsetEnd = p;
                break;
            }
        }

        if (mangledName && offsetBegin && offsetEnd && mangledName < offsetBegin) {
            *mangledName++ = '\0';
            *offsetBegin++ = '\0';
            *offsetEnd = '\0';

            int status;
            char* demangledName = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);

            if (status == 0 && demangledName != nullptr) {
                std::cerr << symbols[i] << " : " << demangledName << "+" << offsetBegin << '\n';
                free(demangledName);
                debugSymbolsIncluded = true;
            } else {
                std::cerr << symbols[i] << " : " << mangledName << "+" << offsetBegin << '\n';
            }
        } else {
            std::cerr << symbols[i] << '\n';
        }
    }

    // Check if debug symbols were found
    if (!debugSymbolsIncluded) {
        std::cerr << "Warning: Debugging symbols were not included; stack trace may be less informative.\n";
    }

    free(symbols);
#endif
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