#pragma once
#ifndef MMWAVE_H
#define MMWAVE_H
#include <vector>
#include <string>
#include <logger.h>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QCoreApplication>
#include <QIODevice>

class mmWave{
    QSerialPort* port;

public:
    mmWave();
    ~mmWave();
};

#endif// MMWAVE_H
