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

Other info stored in EERPOM
- Wifi creds that the user can use to configure the device in the event they do not have a BT device
- SSH creds
- Buildnumber
- Software version


*/
