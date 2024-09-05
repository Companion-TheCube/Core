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
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#include <iostream>
#include <memory>
#include <latch>
#include "authentication.h"
#include <utils.h>
// #include "api_i.h"

class Endpoint {
private:
    std::string name;
    std::string path;
    
    std::function<std::string(const httplib::Request &req, httplib::Response &res)> action;
public:

int endpointType;
    Endpoint(int endpointType, std::string name, std::string path);
    ~Endpoint();
    std::string getName();
    std::string getPath();
    bool isPublic() const;
    bool isGetType() const;
    void setAction(std::function<std::string(const httplib::Request &req, httplib::Response &res)> action);
    std::string doAction(const httplib::Request &req, httplib::Response &res);
    std::function<std::string(const httplib::Request &req, httplib::Response &res)> getAction();
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
    void addEndpoint(bool isGetType, std::string path, std::function<void(const httplib::Request&, httplib::Response&)> action);
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
    CubeAuth *auth;
    void httpApiThreadFn();
public:
    API();
    ~API();
    void start();
    void stop();
    void restart();
    void addEndpoint(std::string name, std::string path, int endpointType, std::function<std::string(const httplib::Request &req, httplib::Response &res)> action);
    std::vector<Endpoint*> getEndpoints();
    Endpoint* getEndpointByName(std::string name);
    bool removeEndpoint(std::string name);
};

