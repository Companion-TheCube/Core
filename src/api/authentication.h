#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <logger.h>
#include "./../database/db.h"

std::string KeyGenerator(size_t length);
std::string Code6Generator();