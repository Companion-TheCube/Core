/*
███╗   ███╗ █████╗ ██╗███╗   ██╗   ██╗  ██╗
████╗ ████║██╔══██╗██║████╗  ██║   ██║  ██║
██╔████╔██║███████║██║██╔██╗ ██║   ███████║
██║╚██╔╝██║██╔══██║██║██║╚██╗██║   ██╔══██║
██║ ╚═╝ ██║██║  ██║██║██║ ╚████║██╗██║  ██║
╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝╚═╝╚═╝  ╚═╝
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

// #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdlib>
#ifdef __linux__
#include <cstdlib>
#include <cxxabi.h>
#include <execinfo.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "api/api.h"
#include "api/authentication.h"
#include "apps/appsManager.h"
#include "argparse.hpp"
#include "audio/audioManager.h"
#include "database/cubeDB.h"
#include "gui/gui.h"
#include "hardware/bluetooth.h"
#include "hardware/peripheralManager.h"
#include "hardware/wifi.h"
#include "decisionEngine/decisions.h"
#include "settings/loader.h"
#include <chrono>
#include <cmath>
#include <csignal>
#include <functional>
#include <future>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <utils.h>

bool supportsBasicColors();
bool supportsExtendedColors();
void signalHandler(int signum);
void printStackTrace();