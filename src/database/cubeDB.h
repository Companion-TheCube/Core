/*
 ██████╗██╗   ██╗██████╗ ███████╗██████╗ ██████╗    ██╗  ██╗
██╔════╝██║   ██║██╔══██╗██╔════╝██╔══██╗██╔══██╗   ██║  ██║
██║     ██║   ██║██████╔╝█████╗  ██║  ██║██████╔╝   ███████║
██║     ██║   ██║██╔══██╗██╔══╝  ██║  ██║██╔══██╗   ██╔══██║
╚██████╗╚██████╔╝██████╔╝███████╗██████╔╝██████╔╝██╗██║  ██║
 ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝╚═════╝ ╚═════╝ ╚═╝╚═╝  ╚═╝
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

#pragma once
#ifndef CUBEDB_H
#define CUBEDB_H
#ifndef DB_H
#include "db.h"
#endif
#ifndef API_I_H
#include "../api/api.h"
#endif
#include "nlohmann/json.hpp"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utils.h>

class CubeDB : public AutoRegisterAPI<CubeDB> {
    static std::shared_ptr<CubeDatabaseManager> dbManager;
    static std::shared_ptr<BlobsManager> blobsManager;
    static bool isDBManagerSet;
    static bool isBlobsManagerSet;

public:
    CubeDB(std::shared_ptr<CubeDatabaseManager> dbManager, std::shared_ptr<BlobsManager> blobsManager);
    /**
     * @brief Construct a new CubeDB object. Must call setCubeDBManager and setBlobsManager before using this object.
     */
    CubeDB() {};
    ~CubeDB() {};
    static void setCubeDBManager(std::shared_ptr<CubeDatabaseManager> dbManager);
    static void setBlobsManager(std::shared_ptr<BlobsManager> blobsManager);
    static std::shared_ptr<CubeDatabaseManager> getDBManager();
    static std::shared_ptr<BlobsManager> getBlobsManager();
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;
};


#endif // CUBEDB_H
