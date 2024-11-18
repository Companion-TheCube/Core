#include "mmWave.h"

#ifndef ___linux__
int wiringPiSetup() { return -1; }
int serialOpen(const char* port, int baud) { return -1; }
void serialPuts(int port, char* data) { }
int serialDataAvail(int port) { return 0; }
char serialGetchar(int port) { return 0; }
unsigned long millis() { return 0; }
#endif

mmWave::mmWave()
{
    if (wiringPiSetup() == -1) {
        CubeLog::error("Failed to setup wiringPi.");
        return;
    }
    this->readerThread = std::make_unique<std::jthread>([&](std::stop_token st) {
        this->serialPort_h = serialOpen("/dev/ttyAMA4", 115200);
        if (serialPort_h < 0) {
            CubeLog::error("Failed to open serial port.");
            while (!st.stop_requested()) {
                genericSleep(1000);
                CubeLog::info("Failure.");
            }
        }
        // this->enableConfigMode();
        // this->enableEngineeringMode();
        // this->disableConfigMode();
        unsigned long lastPrintTime = millis();
        while (!st.stop_requested()) {
            Response response = readDataFrame();
            if (response.success) {
                // CubeLog::info("Data received: " + response.hexStr);
                decodeDataFrame(response);
            }
            if (millis() - lastPrintTime > 10000) {
                CubeLog::info("Target State: " + std::to_string(this->targetState));
                CubeLog::info("Moving Target Distance: " + std::to_string(this->movingTargetDistance));
                CubeLog::info("Moving Target Energy: " + std::to_string(this->movingTargetEnergy));
                CubeLog::info("Stationary Target Distance: " + std::to_string(this->stationaryTargetDistance));
                CubeLog::info("Stationary Target Energy: " + std::to_string(this->stationaryTargetEnergy));
                CubeLog::info("Detection Distance: " + std::to_string(this->detectionDistance));
                lastPrintTime = millis();
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
    while (serialDataAvail(this->serialPort_h) == 0 && waitTime < 1000) {
        genericSleep(10);
        waitTime += 10;
    }
    if (waitTime >= 1000) {
        CubeLog::error("Timeout trying to get response.");
        response = false;
        return response;
    }
    // These nested loops get the data fast, but if we read faster than the data arrives, give  the source
    // time to catch up.
    while (serialDataAvail(this->serialPort_h) > 0) {
        while (serialDataAvail(this->serialPort_h) > 0) {
            response += serialGetchar(this->serialPort_h);
        }
        genericSleep(10);
    }
    return response;
}

Response mmWave::sendCommand(std::vector<uint8_t> command, std::vector<uint8_t> ack)
{
    Response response = this->sendCommand(command);
    if (!response.success || response != ack) {
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
    if (response.success) {
        this->configModeEnabled = true;
    } else {
        CubeLog::error("Failed to enable command mode.");
        this->configModeEnabled = false;
    }
    return response.success;
}

bool mmWave::disableConfigMode()
{
    std::vector<uint8_t> commandMode = COMMAND_HEADER;
    const std::vector<uint8_t> commandData = DISABLE_CONFIG_MODE;
    commandMode.push_back(commandData.size() & 0xFF);
    commandMode.push_back((commandData.size() >> 8) & 0xFF);
    commandMode.insert(commandMode.end(), commandData.begin(), commandData.end());
    commandMode.insert(commandMode.end(), COMMAND_TAIL);

    std::vector<uint8_t> ack = COMMAND_HEADER;
    const std::vector<uint8_t> ackData = DISABLE_CONFIG_MODE_ACK;
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
    response = true;
    int headerSize = response.size();
    if (this->configModeEnabled) {
        CubeLog::error("Command mode enabled.");
        response = false;
        return response;
    }
    uint16_t waitTime = 0;
    while (serialDataAvail(this->serialPort_h) == 0 && waitTime < 1000) {
        genericSleep(10);
        waitTime += 10;
    }
    if (waitTime >= 1000) {
        CubeLog::error("Timeout trying to get data frame.");
        response = false;
        return response;
    }
    genericSleep(1);
    if (serialDataAvail(this->serialPort_h) == 0) {
        CubeLog::error("No data available.");
        response = false;
        return response;
    }
    int headerIndex = 0;
    genericSleep(1);
    while (serialDataAvail(this->serialPort_h) > 0) {
        uint8_t c = serialGetchar(this->serialPort_h);
        if (c == response[headerIndex]) {
            headerIndex++;
            if (headerIndex == response.size()) {
                break;
            }
        } else {
            headerIndex = 0;
        }
    }
    if (headerIndex != response.size()) {
        CubeLog::error("Failed to find report header.");
        response = false;
        return response;
    }

    genericSleep(1);
    uint16_t reportSize = 0;
    reportSize = serialGetchar(this->serialPort_h);
    reportSize |= serialGetchar(this->serialPort_h) << 8;

    response += reportSize & 0xFF;
    response += (reportSize >> 8) & 0xFF;

    if (serialDataAvail(this->serialPort_h) < reportSize) {
        CubeLog::error("Report size does not match data available.");
        response = false;
        return response;
    }

    genericSleep(2);
    std::vector<uint8_t> tail = REPORT_TAIL;
    for (uint16_t i = 0; i < reportSize + tail.size(); i++) {
        response += serialGetchar(this->serialPort_h);
    }

    if (response.size() != reportSize + tail.size() + headerSize + 2) { // 2 for the size bytes
        CubeLog::error("Failed to get full report.");
        response = false;
        return response;
    }

    for (uint16_t i = 1; i <= tail.size(); i++) {
        if (response[response.size() - i] != tail[tail.size() - i]) {
            CubeLog::error("Failed to find report tail.");
            response = false;
            return response;
        }
    }

    return response;
}

void mmWave::decodeDataFrame(Response response)
{
    if (!response.success) {
        CubeLog::error("Failed to decode data frame.");
        return;
    }
    // CubeLog::debugSilly("Encoded frame: " + response.hexStr);
    std::vector<uint8_t> data = response.getData();

    // datalength, two bytes, little endian
    uint16_t dataLength = data[0] | data[1] << 8;

    // Data type, Normal or Engineering
    uint8_t dataType = data[2];

    // Head
    uint8_t head = data[3];

    if (head != 0xaa) {
        CubeLog::error("Failed to find head.");
        return;
    }

    // Tail
    uint8_t tail = data.at(data.size() - 2);

    if (tail != 0x55) {
        CubeLog::error("Failed to find tail.");
        return;
    }

    // check
    uint8_t check = data.at(data.size() - 1);

    if (dataType == 0x02) {
        // CubeLog::info("Normal data frame.");
        if (data.size() != 15) {
            CubeLog::error("Data size does not match.");
            return;
        }
        // target state, 1 byte
        this->targetState = data[4];
        // moving target distance, 2 bytes, cm
        this->movingTargetDistance = data[5] | data[6] << 8;
        // Excercise target energy value, 1 byte
        this->movingTargetEnergy = data[7];
        // Stationary target distance, 2 byte, cm
        this->stationaryTargetDistance = data[8] | data[9] << 8;
        // Stationary target energy, 1 byte
        this->stationaryTargetEnergy = data[10];
        // Detection Distance, 2 byte, cm
        this->detectionDistance = data[11] | data[12] << 8;
    } else if (dataType == 0x01) {
        CubeLog::info("Engineering data frame.");
        if (data.size() != 33) { // TODO: Check this
            CubeLog::error("Data size does not match.");
            return;
        }
        // target state, 1 byte
        this->targetState = data[4];
        // Moving target distance, 2 bytes, cm
        this->movingTargetDistance = data[5] | data[6] << 8;
        // Moving target energy value, 1 byte
        this->movingTargetEnergy = data[7];
        // Stationary target distance, 2 byte, cm
        this->stationaryTargetDistance = data[8] | data[9] << 8;
        // Stationary target energy, 1 byte
        this->stationaryTargetEnergy = data[10];
        // Detection Distance, 2 byte, cm
        this->detectionDistance = data[11] | data[12] << 8;
        // Max moving distance gate number, 1 byte
        uint8_t maxMovingDistanceGateNumber = data[13];
        // max stationary distance gate number, 1 byte
        uint8_t maxStationaryDistanceGateNumber = data[14];
        std::vector<uint8_t> movingDistanceGateEnergy;
        movingDistanceGateEnergy.reserve(maxMovingDistanceGateNumber);
        for (size_t i = 0; i < maxMovingDistanceGateNumber; i++) {
            movingDistanceGateEnergy.push_back(data[15 + i]);
        }
        std::vector<uint8_t> stationaryDistanceGateEnergy;
        stationaryDistanceGateEnergy.reserve(maxStationaryDistanceGateNumber);
        for (size_t i = 0; i < maxStationaryDistanceGateNumber; i++) {
            stationaryDistanceGateEnergy.push_back(data[15 + maxMovingDistanceGateNumber + i]);
        }
    } else {
        CubeLog::error("Unknown data type.");
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
