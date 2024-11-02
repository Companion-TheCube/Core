// #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdlib>
#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <execinfo.h>
#include <cxxabi.h>
#endif
#include "api/builder.h"
#include "apps/appsManager.h"
#include "argparse.hpp"
#include "database/cubeDB.h"
#include "gui/gui.h"
#include "logger/logger.h"
#include "settings/loader.h"
#include "audio/audioManager.h"
#include "hardware/wifi.h"
#include "hardware/bluetooth.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <utils.h>
#include <functional>
#include <csignal>
#include <future>
#include <chrono>
#include "doctest.h"

bool supportsBasicColors();
bool supportsExtendedColors();
void signalHandler(int signum);
void printStackTrace();