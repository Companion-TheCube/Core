#include "mmWave.h"

std::vector<std::string> portNames;
std::vector<std::string> portPrefixes;

std::vector<uint8_t> commandMode = { 0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xff, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };
std::vector<uint8_t> commandModeAck = { 0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xff, 0x01, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };

void generatePortNames();

mmWave::mmWave()
{
    generatePortNames();
    if(wiringPiSetup() == -1){
        CubeLog::error("Failed to setup wiringPi.");
        return;
    }
    this->readerThread = new std::jthread([&](std::stop_token st) {
        int serialPort = serialOpen("/dev/ttyAMA4", 115200);
        if(serialPort < 0){
            CubeLog::error("Failed to open serial port.");
            while(!st.stop_requested()){
                genericSleep(1000);
                CubeLog::info("Failure.");
            }
        }
        // serialPuts(serialPort, (char*)commandMode.data());
        
        while(!st.stop_requested()){
            genericSleep(1000);
            while(serialDataAvail(serialPort) == 0){
                genericSleep(1000);
            }
            std::string responseStr;
            while(serialDataAvail(serialPort) > 0){
                responseStr += serialGetchar(serialPort);
            }
            CubeLog::debugSilly("response length: " + std::to_string(responseStr.length()));
            // create a vector of uint8_t from the string
            std::vector<uint8_t> response(responseStr.begin(), responseStr.end());
            // print the data as hex values
            std::string hexStr;
            std::ostringstream oss;
            for(auto c: response){
                oss << std::hex 
                << std::setw(2) 
                << std::setfill('0') 
                << std::uppercase
                << (int)c << " ";
            }
            hexStr = oss.str();
            CubeLog::debugSilly("response: " + hexStr);
        }
    });
}

mmWave::~mmWave()
{
    delete this->readerThread;
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