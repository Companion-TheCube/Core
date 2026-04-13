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
#include "utils.h"

bool breakMain = false;

int main(int argc, char* argv[])
{
#ifndef __linux__
    static_assert(false, "This code is only for Linux.");
#endif
    // Load configuration from .env once (available globally via Config::get())
    Config::loadFromDotEnv(".env");
    {
        std::error_code execEc;
        const auto executablePath = std::filesystem::read_symlink("/proc/self/exe", execEc);
        if (!execEc && !executablePath.empty()) {
            const auto executableDir = executablePath.parent_path();
            std::error_code shadersEc;
            if (std::filesystem::exists(executableDir / "shaders", shadersEc)) {
                std::error_code chdirEc;
                std::filesystem::current_path(executableDir, chdirEc);
                if (chdirEc) {
                    std::cerr << "Failed to change working directory to executable directory: " << chdirEc.message() << std::endl;
                }
            }
        }
    }
    // std::signal(SIGINT, signalHandler);
    // std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGILL, signalHandler);
    std::signal(SIGFPE, signalHandler);
    std::signal(SIGKILL, signalHandler);
    std::signal(SIGQUIT, signalHandler);
    std::signal(SIGBUS, signalHandler);
    std::signal(SIGSYS, signalHandler);
    std::signal(SIGPIPE, signalHandler);
    std::signal(SIGALRM, signalHandler);
    std::signal(SIGHUP, signalHandler);

    std::cout << "Starting..." << std::endl;

    // TODO: rather than set this environment variable in this application, set it in the manager application. The manager app
    // should also verify if a symbolic link is needed and create it if necessary.
    if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr) {
        if (setenv("DISPLAY", ":0", 1) != 0) {
            std::cout << "Error setting DISPLAY=:0 environment variable. Exiting." << std::endl;
            return 1;
        }
    }

    /////////////////////////////////////////////////////////////////
    // Argument parsing
    /////////////////////////////////////////////////////////////////
    std::string versionInfo = "Companion, TheCube - CORE ver: ";
    versionInfo += SW_VERSION;
    versionInfo += "\n";
    versionInfo += "Built on: " + std::string(__DATE__) + " " + std::string(__TIME__) + "\n";
    versionInfo += "Built on: ";
    versionInfo += "Linux";
    versionInfo += "\nCompiler: ";
#ifdef __clang__
    versionInfo += "Clang";
#endif
#ifdef __GNUC__
    versionInfo += "GCC";
#endif
    versionInfo += "\nC++ Standard: ";
#ifdef __cplusplus
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
    auto settingsLoader = std::make_unique<SettingsLoader>(&settings);
    settingsLoader->loadSettings();
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
        auto db_manager = std::make_shared<CubeDatabaseManager>();
        auto blobs = std::make_shared<BlobsManager>(db_manager, "data/blobs.db");
        auto cubeDB = std::make_shared<CubeDB>(db_manager, blobs);
        AppsManager appsManager;

        // TODO: App install/upgrade flows should materialize manifests under
        // APP_INSTALL_ROOTS. AppsManager::initialize() now owns manifest sync
        // plus best-effort startup of enabled system apps and autostart apps
        // through launcher/systemd.
        if (!appsManager.initialize()) {
            CubeLog::warning("AppsManager initialization completed with one or more manifest sync errors.");
        }
        
        CubeLog::info("Loading GUI...");
        auto gui = std::make_shared<GUI>();
        auto audioManager = std::make_shared<AudioManager>();
        auto wifiManager = std::make_shared<WifiManager>();
        // auto btManager = std::make_shared<BTManager>();
        // Testing /////////////////////////////////////////////////
        long blobID = CubeDB::getBlobsManager()->addBlob("client_blobs", "test blob", "1");
        CubeLog::info("Blob ID: " + std::to_string(blobID));
        // end testing //////////////////////////////////////////////
        auto api = std::make_shared<API>();
        auto auth = std::make_shared<CubeAuth>();
        auto appPostgresAccess = std::make_shared<AppPostgresAccess>();
        auto peripherals = std::make_shared<PeripheralManager>();
        auto apiEventBroker = api->getEventBroker();
        auto interactionEventBridge = std::make_shared<InteractionEventBridge>(apiEventBroker);
        auto interactionApi = std::make_shared<InteractionAPI>(peripherals, apiEventBroker);
        auto eventsApi = std::make_shared<EventsAPI>(api);
        auto decisions = std::make_shared<DecisionEngine::DecisionEngineMain>();

        API_Builder api_builder(api);
        gui->registerInterface();
        cubeDB->registerInterface();
        logger->registerInterface();
        audioManager->registerInterface();
        auth->registerInterface();
        appPostgresAccess->registerInterface();
        peripherals->registerInterface();
        interactionApi->registerInterface();
        eventsApi->registerInterface();
        decisions->registerInterface();
        // btManager->registerInterface();
        api_builder.start();
        decisions->start();

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
#ifndef PRODUCTION_BUILD
        cpuAndMemoryThread.request_stop();
#endif
        appsManager.shutdown();
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
    default:
        CubeLog::info("Caught unknown signal.");
        breakMain = true;
        break;
    }
}

// Function to print stack trace
void printStackTrace()
{
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
}

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

    // TODO: popen() + fgets() against a shell command is fragile because it depends on shell/process cleanup and unchecked command output; replace with a direct terminfo/terminal capability query or an RAII subprocess wrapper.
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
