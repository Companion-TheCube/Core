#pragma once
#ifndef MMWAVE_H
#define MMWAVE_H
#include <vector>
#include <string>
#include <logger.h>
#ifdef __linux__
#include <wiringPi.h>
#include <wiringSerial.h>
#include <iostream>
#include <unistd.h>
#endif

// TODO: add some ifdefs and defines for the port name on RasPi
// TODO: add ifdefs for Windows so that this will compile on Windows
// For windows, we should just mock the mmWave sensor.

class mmWave{
    std::jthread* readerThread;
public:
    mmWave();
    ~mmWave();
};

#endif// MMWAVE_H
