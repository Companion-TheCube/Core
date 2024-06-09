#include "utils.h"

void genericSleep(int ms)
{
#ifdef __linux__
    usleep(ms * 1000);
#endif
#ifdef _WIN32
    Sleep(ms);
#endif
}