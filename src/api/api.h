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

typedef std::vector<std::pair<std::string, std::string>> EndPointParams_t;
typedef std::vector<std::pair<bool,std::function<std::string(std::string, EndPointParams_t)>>> EndPointData_t;


class I_API_Interface {
public:
    virtual ~I_API_Interface() = default;
    virtual std::string getIntefaceName() const = 0;
    /**
     * @brief Get the Endpoint Data object
     * 
     * @return std::vector<std::pair<bool,std::function<std::string(std::string, std::vector<std::pair<std::string, std::string>>)>>>
     * The function element of the pair is the action to be executed when the endpoint is hit and
     * should return as soon as possible. This function will be executed on the API thread and should
     * not block the thread. Make sure that the function is thread safe. The bool element of the pair
     * is whether the endpoint is public or not. If the endpoint is public, it will be accessible from
     * any client on the network. If the endpoint is not public, it will only be accessible only from
     * authenticated clients.
     **/
    virtual EndPointData_t getEndpointData() = 0;
    virtual std::vector<std::string> getEndpointNames() = 0;
};

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
    void addEndpoint(std::string name, std::string path, bool publicEndpoint, std::function<std::string(std::string response, std::vector<std::pair<std::string, std::string>> params)> action);
    std::vector<Endpoint*> getEndpoints();
    Endpoint* getEndpointByName(std::string name);
    bool removeEndpoint(std::string name);
    void httpApiThreadFn();
};

