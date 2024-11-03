#include "mmWave.h"

std::vector<std::string> portNames;
std::vector<std::string> portPrefixes;

std::vector<uint8_t> commandMode = { 0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xff, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };
std::vector<uint8_t> commandModeAck = { 0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xff, 0x01, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };

void generatePortNames();

mmWave::mmWave()
{
    generatePortNames();
    this->port = new QSerialPort();
    for(auto p : portNames){
        this->port->setPortName(p.c_str());
        if(this->port->open(QIODevice::ReadWrite)){
            CubeLog::info("Opened port: " + p);
            break;
        }
    }
    this->port->setBaudRate(QSerialPort::Baud57600);
    this->port->setDataBits(QSerialPort::Data8);
    this->port->setParity(QSerialPort::NoParity);
    this->port->setStopBits(QSerialPort::OneStop);
    this->port->setFlowControl(QSerialPort::NoFlowControl);
    this->port->write((char*)commandMode.data(), commandMode.size());
    this->port->waitForBytesWritten(1000);
    this->port->waitForReadyRead(1000);
    QByteArray response = this->port->readAll();
    if(response.size() == commandModeAck.size()){
        bool ack = true;
        for(int i = 0; i < response.size(); i++){
            if(response[i] != commandModeAck[i]){
                ack = false;
                break;
            }
        }
        CubeLog::debugSilly("response: " + response.toStdString());
        if(ack){
            CubeLog::info("mmWave sensor is in command mode.");
        }else{
            CubeLog::error("mmWave sensor did not respond with the correct ack.");
        }
    }else{
        CubeLog::error("mmWave sensor did not respond with the correct ack.");
    }

    std::jthread readerThread([&] {
        while(true){
            this->port->waitForReadyRead(1000);
            QByteArray response = this->port->readAll();
            CubeLog::debugSilly("response: " + response.toStdString());
        }
    });

}

mmWave::~mmWave()
{
}


////////////////////////////////////////////////////////////////////////////////////////

void generatePortNames(){
#ifdef _WIN32
    portPrefixes.push_back("COM");
#else
    portPrefixes.push_back("/dev/ttyACM"); 
    portPrefixes.push_back("/dev/ttyUSB"); 
    portPrefixes.push_back("/dev/ttyS");
#endif
    for(auto p : portPrefixes){
        for(int i = 0; i < 50; i++){
            portNames.push_back(p + std::to_string(i));
        }
    }
}