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
#include "api/builder.h"
#include "apps/appsManager.h"
#include "argparse.hpp"
#include "audio/audioManager.h"
#include "database/cubeDB.h"
#include "doctest.h"
#include "gui/gui.h"
#include "hardware/bluetooth.h"
#include "hardware/peripheralManager.h"
#include "hardware/wifi.h"
#include "decisionEngine/decisions.h"
#include "logger/logger.h"
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