#include <cstdlib>
#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#include "api/builder.h"
#include "apps/appsManager.h"
#include "argparse.hpp"
#include "database/cubeDB.h"
#include "gui/gui.h"
#include "logger/logger.h"
#include "settings/loader.h"
#include <RtAudio.h>
#include <cmath>
#include <functional>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <utils.h>
#include <functional>

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

#else
#include <cstdlib>
#include <string>

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