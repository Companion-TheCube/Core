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

class mmWave{

public:
    mmWave();
    ~mmWave();
};

#endif// MMWAVE_H
