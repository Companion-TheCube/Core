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

class I_API_Interface {
public:
    virtual ~I_API_Interface() = default;
    virtual std::string getIntefaceName() const = 0;
    virtual std::vector<std::pair<bool,std::function<void()>>> getEndpointData() = 0; // bool: public, function: action
    virtual std::vector<std::string> getEndpointNames() = 0;
};

class Endpoint {
private:
    std::string name;
    std::string path;
    bool publicEndpoint;
    std::function<void()> action;
public:
    Endpoint(bool publicEndpoint, std::string name, std::string path);
    ~Endpoint();
    std::string getName();
    std::string getPath();
    bool isPublic();
    void setAction(std::function<void()> action);
    void doAction();
    std::function<void()> getAction();
};

class CubeHttpServer {
private:
    httplib::Server* server;
    CubeLog *logger;
    std::string address;
    int port;
public:
    CubeHttpServer(CubeLog *logger, std::string address, int port);
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
    CubeLog *logger;
    std::jthread listenerThread;
    CubeHttpServer *server;
    std::vector<std::pair<std::string, bool>> endpointTriggers;
public:
    API(CubeLog *logger);
    ~API();
    void start();
    void stop();
    void restart();
    void addEndpoint(std::string name, std::string path, bool publicEndpoint, std::function<void()> action);
    std::vector<Endpoint*> getEndpoints();
    Endpoint* getEndpointByName(std::string name);
    bool removeEndpoint(std::string name);
    void httpApiThreadFn();
};

