#include <cstdlib>
#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
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

int saw(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData);
bool supportsBasicColors();
bool supportsExtendedColors();