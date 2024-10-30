// #pragma once
#ifndef API_H
#define API_H
#include <functional>
#include <logger.h>
#include <string>
#include <thread>
#include <vector>
#ifndef HTTPLIB_INCLUDED
#define HTTPLIB_INCLUDED
#include <httplib.h>
#endif
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
#include <latch>
#include <memory>
#ifndef AUTHENTICATION_H
#include "authentication.h"
#endif
#include <utils.h>
#ifndef API_I_H
#include "api_i.h"
#endif

#define CUBE_SOCKET_PATH "cube.sock"

class Endpoint {
private:
    std::string name;
    std::string path;

    EndpointAction_t action;

public:
    int endpointType;
    Endpoint(int endpointType, const std::string& name, const std::string& path);
    ~Endpoint();
    std::string getName();
    std::string getPath();
    bool isPublic() const;
    bool isGetType() const;
    void setAction(EndpointAction_t action);
    EndpointError doAction(const httplib::Request& req, httplib::Response& res);
    EndpointAction_t getAction();
};

class CubeHttpServer {
private:
    std::shared_ptr<httplib::Server> server;
    std::string address;
    int port;
    std::unique_ptr<std::jthread> serverThread;

public:
    CubeHttpServer(const std::string& address, int port);
    ~CubeHttpServer();
    void start();
    void stop();
    void restart();
    void addEndpoint(bool isGetType, const std::string& path, std::function<void(const httplib::Request&, httplib::Response&)> action);
    void removeEndpoint(const std::string& path);
    void setPort(int port);
    int getPort();
    std::shared_ptr<httplib::Server> getServer();
};

class API {
private:
    std::vector<std::shared_ptr<Endpoint>> endpoints = {};
    std::jthread listenerThread;
    std::unique_ptr<CubeHttpServer> server;
    std::unique_ptr<CubeHttpServer> serverIPC;
    std::vector<std::pair<std::string, bool>> endpointTriggers = {};
    void httpApiThreadFn();

public:
    API();
    ~API();
    void start();
    void stop();
    void restart();
    void addEndpoint(const std::string& name, const std::string& path, int endpointType, EndpointAction_t action);
    std::vector<std::shared_ptr<Endpoint>> getEndpoints();
    std::shared_ptr<Endpoint> getEndpointByName(const std::string& name);
    bool removeEndpoint(const std::string& name);
};

#endif // API_H
