#include "systemCtrl.h"

// Shutdown, reboot, and factory reset are all handled by the systemCtrl class.
// The actions this class performs shall only be available through the GUI. (No API endpoints)
// Hardware monitoring is handled by the hwMonitor class. (CPU usage, memory usage, temperature, etc.)
// This class shall have API endpoints for the following:
// - Get the current CPU usage per app
// - Get the current memory usage per app
// - Get the current total CPU usage
// - Get the current total memory usage
// - Get the current CPU temperature
// - Adjust the CPU fan speed
