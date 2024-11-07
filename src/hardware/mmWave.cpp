#include "mmWave.h"

std::vector<uint8_t> commandMode = { 0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xff, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };
std::vector<uint8_t> commandModeAck = { 0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xff, 0x01, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01 };

#ifdef _WIN32
int wiringPiSetup() { return -1; }
int serialOpen(const char* port, int baud) { return -1; }
void serialPuts(int port, char* data) {}
int serialDataAvail(int port) { return 0; }
char serialGetchar(int port) { return 0; }
#endif

mmWave::mmWave()
{
    if(wiringPiSetup() == -1){
        CubeLog::error("Failed to setup wiringPi.");
        return;
    }
    this->readerThread = std::make_unique<std::jthread>([&](std::stop_token st) {
        this->serialPort_h = serialOpen("/dev/ttyAMA4", 115200);
        if(serialPort_h < 0){
            CubeLog::error("Failed to open serial port.");
            while(!st.stop_requested()){
                genericSleep(1000);
                CubeLog::info("Failure.");
            }
        }
        // serialPuts(serialPort_h, (char*)commandMode.data());
        
        while(!st.stop_requested()){
            Response response = readDataFrame();
            if(response.success){
                CubeLog::info("Data received: " + response.hexStr);
                decodeDataFrame(response);
            }
        }
    });
}

mmWave::~mmWave()
{
    this->readerThread->request_stop();
    this->readerThread->join();
}

Response mmWave::sendCommand(std::vector<uint8_t> command)
{
    std::vector<uint8_t> dataToSend = COMMAND_HEADER;
    dataToSend.push_back(command.size() & 0xFF);
    dataToSend.push_back((command.size() >> 8) & 0xFF);
    dataToSend.insert(dataToSend.end(), command.begin(), command.end());
    dataToSend.insert(dataToSend.end(), COMMAND_TAIL);
    serialPuts(this->serialPort_h, (char*)dataToSend.data());
    Response response;
    uint16_t waitTime = 0;
    while(serialDataAvail(this->serialPort_h) == 0 && waitTime < 1000){
        genericSleep(10);
        waitTime += 10;
    }
    if(waitTime >= 1000){
        CubeLog::error("Timeout trying to get response.");
        response = false;
        return response;
    }
    // These nested loops get the data fast, but if we read faster than the data arrives, give  the source
    // time to catch up.
    while(serialDataAvail(this->serialPort_h) > 0){
        while(serialDataAvail(this->serialPort_h) > 0){
            response += serialGetchar(this->serialPort_h);
        }
        genericSleep(10);
    }
    return response;
}

Response mmWave::sendCommand(std::vector<uint8_t> command, std::vector<uint8_t> ack)
{
    Response response = this->sendCommand(command);
    if(!response.success || response != ack){
        CubeLog::error("Failed to get expected response.");
        response = false;
        return response;
    }
    return response;
}

bool mmWave::enableConfigMode()
{
    std::vector<uint8_t> commandMode = COMMAND_HEADER;
    std::vector<uint8_t> commandData = ENABLE_CONFIG_MODE;
    commandMode.push_back(commandData.size() & 0xFF);
    commandMode.push_back((commandData.size() >> 8) & 0xFF);
    commandMode.insert(commandMode.end(), commandData.begin(), commandData.end());
    commandMode.insert(commandMode.end(), COMMAND_TAIL);

    std::vector<uint8_t> ack = COMMAND_HEADER;
    std::vector<uint8_t> ackData = ENABLE_CONFIG_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandMode, ack);
    return response.success;
}

bool mmWave::disableConfigMode()
{
    std::vector<uint8_t> commandMode = COMMAND_HEADER;
    std::vector<uint8_t> commandData = DISABLE_CONFIG_MODE;
    commandMode.push_back(commandData.size() & 0xFF);
    commandMode.push_back((commandData.size() >> 8) & 0xFF);
    commandMode.insert(commandMode.end(), commandData.begin(), commandData.end());
    commandMode.insert(commandMode.end(), COMMAND_TAIL);

    std::vector<uint8_t> ack = COMMAND_HEADER;
    std::vector<uint8_t> ackData = DISABLE_CONFIG_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandMode, ack);
    return response.success;
}

bool mmWave::enableEngineeringMode()
{
    std::vector<uint8_t> commandMode = COMMAND_HEADER;
    std::vector<uint8_t> commandData = ENABLE_ENGINEERING_MODE;
    commandMode.push_back(commandData.size() & 0xFF);
    commandMode.push_back((commandData.size() >> 8) & 0xFF);
    commandMode.insert(commandMode.end(), commandData.begin(), commandData.end());
    commandMode.insert(commandMode.end(), COMMAND_TAIL);

    std::vector<uint8_t> ack = COMMAND_HEADER;
    std::vector<uint8_t> ackData = ENABLE_ENGINEERING_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandMode, ack);
    return response.success;
}

bool mmWave::disableEngineeringMode()
{
    std::vector<uint8_t> commandMode = COMMAND_HEADER;
    std::vector<uint8_t> commandData = DISABLE_ENGINEERING_MODE;
    commandMode.push_back(commandData.size() & 0xFF);
    commandMode.push_back((commandData.size() >> 8) & 0xFF);
    commandMode.insert(commandMode.end(), commandData.begin(), commandData.end());
    commandMode.insert(commandMode.end(), COMMAND_TAIL);

    std::vector<uint8_t> ack = COMMAND_HEADER;
    std::vector<uint8_t> ackData = DISABLE_ENGINEERING_MODE_ACK;
    ack.push_back(ackData.size() & 0xFF);
    ack.push_back((ackData.size() >> 8) & 0xFF);
    ack.insert(ack.end(), ackData.begin(), ackData.end());
    ack.insert(ack.end(), COMMAND_TAIL);

    Response response = this->sendCommand(commandMode, ack);
    return response.success;
}

Response mmWave::readDataFrame()
{
    Response response;
    response.data = REPORT_HEADER;
    response = false;
    if(this->commandModeEnabled){
        CubeLog::error("Command mode enabled.");
        return response;
    }
    uint16_t waitTime = 0;
    while(serialDataAvail(this->serialPort_h) == 0 && waitTime < 1000){
        genericSleep(10);
        waitTime += 10;
    }
    if(waitTime >= 1000){
        CubeLog::error("Timeout trying to get data frame.");
        response = false;
        return response;
    }
    if(serialDataAvail(this->serialPort_h) == 0){
        CubeLog::error("No data available.");
        response = false;
        return response;
    }
    int headerIndex = 0;
    while(serialDataAvail(this->serialPort_h) > 0){
        uint8_t c = serialGetchar(this->serialPort_h);
        if(c == response[headerIndex]){
            headerIndex++;
            if(headerIndex == response.size()){
                break;
            }
        } else {
            headerIndex = 0;
        }
    }
    if(headerIndex != response.size()){
        CubeLog::error("Failed to find report header.");
        response = false;
        return response;
    }

    uint16_t reportSize = 0;
    reportSize = serialGetchar(this->serialPort_h);
    reportSize |= serialGetchar(this->serialPort_h) << 8;

    if(serialDataAvail(this->serialPort_h) < reportSize){
        CubeLog::error("Report size does not match data available.");
        response = false;
        return response;
    }

    std::vector<uint8_t> tail = REPORT_TAIL;
    for(uint16_t i = 0; i < reportSize + tail.size(); i++){
        response += serialGetchar(this->serialPort_h);
    }

    if(response.size() != reportSize + tail.size()){
        CubeLog::error("Failed to get full report.");
        response = false;
        return response;
    }

    for(uint16_t i = 0; i < tail.size(); i++){
        if(response[reportSize + i] != tail[i]){
            CubeLog::error("Failed to find report tail.");
            response = false;
            return response;
        }
    }

    return response;
}

void mmWave::decodeDataFrame(Response response)
{
    if(!response.success){
        CubeLog::error("Failed to decode data frame.");
        return;
    }
    std::vector<uint8_t> data = response.data;
    uint16_t index = 0;
    uint16_t reportSize = data[index++];
    reportSize |= data[index++] << 8;
    uint16_t frameNumber = data[index++];
    frameNumber |= data[index++] << 8;
    uint16_t numDetectedObj = data[index++];
    numDetectedObj |= data[index++] << 8;
    uint16_t numTLVs = data[index++];
    numTLVs |= data[index++] << 8;
    CubeLog::info("Report size: " + std::to_string(reportSize));
    CubeLog::info("Frame number: " + std::to_string(frameNumber));
    CubeLog::info("Number of detected objects: " + std::to_string(numDetectedObj));
    CubeLog::info("Number of TLVs: " + std::to_string(numTLVs));
    for(uint16_t i = 0; i < numDetectedObj; i++){
        uint16_t rangeIdx = data[index++];
        rangeIdx |= data[index++] << 8;
        uint16_t dopplerIdx = data[index++];
        dopplerIdx |= data[index++] << 8;
        uint16_t peakVal = data[index++];
        peakVal |= data[index++] << 8;
        uint16_t x = data[index++];
        x |= data[index++] << 8;
        uint16_t y = data[index++];
        y |= data[index++] << 8;
        uint16_t z = data[index++];
        z |= data[index++] << 8;
        uint16_t velocity = data[index++];
        velocity |= data[index++] << 8;
        uint16_t azimuth = data[index++];
        azimuth |= data[index++] << 8;
        uint16_t elevation = data[index++];
        elevation |= data[index++] << 8;
        CubeLog::info("Range index: " + std::to_string(rangeIdx));
        CubeLog::info("Doppler index: " + std::to_string(dopplerIdx));
        CubeLog::info("Peak value: " + std::to_string(peakVal));
        CubeLog::info("X: " + std::to_string(x));
        CubeLog::info("Y: " + std::to_string(y));
        CubeLog::info("Z: " + std::to_string(z));
        CubeLog::info("Velocity: " + std::to_string(velocity));
        CubeLog::info("Azimuth: " + std::to_string(azimuth));
        CubeLog::info("Elevation: " + std::to_string(elevation));
    }
    for(uint16_t i = 0; i < numTLVs; i++){
        uint16_t tlvType = data[index++];
        tlvType |= data[index++] << 8;
        uint16_t tlvLength = data[index++];
        tlvLength |= data[index++] << 8;
        CubeLog::info("TLV type: " + std::to_string(tlvType));
        CubeLog::info("TLV length: " + std::to_string(tlvLength));
        for(uint16_t j = 0; j < tlvLength; j++){
            CubeLog::info("TLV data: " + std::to_string(data[index++]));
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////
