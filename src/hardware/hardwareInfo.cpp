/*
██╗  ██╗ █████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ███████╗██╗███╗   ██╗███████╗ ██████╗     ██████╗██████╗ ██████╗ 
██║  ██║██╔══██╗██╔══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔════╝██║████╗  ██║██╔════╝██╔═══██╗   ██╔════╝██╔══██╗██╔══██╗
███████║███████║██████╔╝██║  ██║██║ █╗ ██║███████║██████╔╝█████╗  ██║██╔██╗ ██║█████╗  ██║   ██║   ██║     ██████╔╝██████╔╝
██╔══██║██╔══██║██╔══██╗██║  ██║██║███╗██║██╔══██║██╔══██╗██╔══╝  ██║██║╚██╗██║██╔══╝  ██║   ██║   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║██║  ██║██║  ██║██████╔╝╚███╔███╔╝██║  ██║██║  ██║███████╗██║██║ ╚████║██║     ╚██████╔╝██╗╚██████╗██║     ██║     
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝  ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═══╝╚═╝      ╚═════╝ ╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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