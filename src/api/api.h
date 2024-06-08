#pragma once
#include <string>
#include <vector>
#include <functional>
#include <logger.h>
#include <thread>
#include <httplib.h>
#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <iostream>
#include <memory>
#include <latch>
#include "authentication.h"
#include "api_i.h"

class Endpoint {
private:
    std::string name;
    std::string path;
    bool publicEndpoint;
    std::function<std::string(std::string, EndPointParams_t)> action;
public:
    Endpoint(bool publicEndpoint, std::string name, std::string path);
    ~Endpoint();
    std::string getName();
    std::string getPath();
    bool isPublic();
    void setAction(std::function<std::string(std::string, EndPointParams_t)> action);
    std::string doAction(std::string, EndPointParams_t);
    std::function<std::string(std::string, EndPointParams_t)> getAction();
};

class CubeHttpServer {
private:
    httplib::Server* server;
    std::string address;
    int port;
public:
    CubeHttpServer(std::string address, int port);
    ~CubeHttpServer();
    void start();
    void stop();
    void restart();
    void addEndpoint(std::string path, std::function<void(const httplib::Request&, httplib::Response&)> action);
    void removeEndpoint(std::string path);
    void setPort(int port);
    int getPort();
    httplib::Server* getServer();
};

class API {
private:
    std::vector<Endpoint*> endpoints;
    std::jthread listenerThread;
    CubeHttpServer *server;
    std::vector<std::pair<std::string, bool>> endpointTriggers;
public:
    API();
    ~API();
    void start();
    void stop();
    void restart();
    void addEndpoint(std::string name, std::string path, bool publicEndpoint, std::function<std::string(std::string response, std::vector<std::pair<std::string, std::string>> params)> action);
    std::vector<Endpoint*> getEndpoints();
    Endpoint* getEndpointByName(std::string name);
    bool removeEndpoint(std::string name);
    void httpApiThreadFn();
};

