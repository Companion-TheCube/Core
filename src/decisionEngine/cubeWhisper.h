#ifndef CUBEWHISPER_H
#define CUBEWHISPER_H

#include <functional>
#include <httplib.h>
#include <iostream>
#include <string>
#include <thread>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include "nlohmann/json.hpp"
#include "utils.h"
#include "whisper.h"

class CubeWhisper {
public:
    CubeWhisper();
    static std::string transcribe(const std::string& audio);

private:
    std::jthread* transcriberThread;
};

#endif // CUBEWHISPER_H
