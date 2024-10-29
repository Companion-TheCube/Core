#include "bluetooth.h"

// TODO: Implement the Bluetooth functions

/**
 * This file will need to have a class for controller the hardware such as turning BT on and off, 
 * scanning for devices, and connecting to devices.
 * There will also need to be a class for communicating with mobiles devices over BT.
 * Perhaps make a class to communicate with other TheCubes over BT which can allow auto configuring 
 * of the network settings on a new device.
 * 
 */

/**
 * 
 *  TODO: In order to comply with Qt licensing, we will have to have all the code that interfaces 
 * with the Qt bluetooth module in a separate application. This app will then use http over unix sockets 
 * to communicate with the main application.
*/


BTControl::BTControl()
{
    

}

BTControl::~BTControl()
{
}

void BTControl::scanForDevices()
{
}

bool BTControl::connectToDevice(BTDevice& device)
{
    return false;
}

bool BTControl::disconnectFromDevice(BTDevice& device)
{
    return false;
}

bool BTControl::pairWithDevice(BTDevice& device)
{
    return false;
}

std::vector<BTDevice> BTControl::getDevices()
{
    return std::vector<BTDevice>();
}

std::vector<BTDevice> BTControl::getPairedDevices()
{
    return std::vector<BTDevice>();
}

std::vector<BTDevice> BTControl::getConnectedDevices()
{
    return std::vector<BTDevice>();
}

std::vector<BTDevice> BTControl::getAvailableDevices()
{
    return std::vector<BTDevice>();
}




//////////////////////////////////////////////////////////////////////////////////

BTServices::BTServices()
{
}

BTServices::~BTServices()
{
}





//////////////////////////////////////////////////////////////////////////////////

BTManager::BTManager()
{
    this->loopThread = std::jthread([&]() {
        
    });

    this->control = new BTControl();
    this->services = new BTServices();
    
}

BTManager::~BTManager()
{
    
    delete this->control;
    delete this->services;
}