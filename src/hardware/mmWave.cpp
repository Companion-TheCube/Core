#include "mmWave.h"

std::vector<std::string> portNames;
std::vector<std::string> portPrefixes;

std::vector<uint8_t> commandMode = { 0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xff, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };
std::vector<uint8_t> commandModeAck = { 0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xff, 0x01, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };

void generatePortNames();

mmWave::mmWave()
{
    generatePortNames();
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