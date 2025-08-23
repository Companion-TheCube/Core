/*

The IO Bridge is an RP2354 IC connected via SPI that provides I2C, SPI, UART, GPIO, and other interfaces to the Cube.

Though the IO Bridge has its own GPIO pins, we will also be controlling the Raspberry Pi's GPIO pins directly for certain functions.

*/

#include "ioBridge.h"