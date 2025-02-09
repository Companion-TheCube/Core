#include "hardwareInfo.h"

/*

In this file we will read the i2c EEPROM to get hardware information. We then need to provide static methods to get the hardware information.

Hardware information includes:
- Serial number
- Model number
- Model revision
- mmWave sensor availability
- nfc availability
- imu availability

We also need to get everything that the infoware library can provide.

Other info stored in EEPROM (for factory reset):
- Wifi creds that the user can use to configure the device in the event they do not have a BT device
- SSH creds
- Buildnumber
- Software version

Need to have functions that provide for updating the EEPROM with new information.

*/

/*
std::string("Hardware Version: 1.0.0\nInfoware version: " + std::string(iware::version)).c_str()
*/