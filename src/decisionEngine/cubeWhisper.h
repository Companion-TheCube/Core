#ifndef CUBEWHISPER_H
#define CUBEWHISPER_H

#include <string>
#include <iostream>
#include <thread>
#include <thread>
#include <functional>
#include <httplib.h>
#include <logger.h>
#include "utils.h"
#include "nlohmann/json.hpp"
#include "whisper.h"

class Whisper{
public:
    Whisper();
    static std::string transcribe(const std::string& audio);
private:
    std::jthread* transcriberThread;
};



#endif// CUBEWHISPER_H
