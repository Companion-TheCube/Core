#include "peripheralManager.h"

// TODO: Implement the peripheral manager. This class will be responsible for managing all the hardware and providing the API endpoints.

PeripheralManager::PeripheralManager()
{
    this->mmWaveSensor = std::make_unique<mmWave>();
}

PeripheralManager::~PeripheralManager()
{
}