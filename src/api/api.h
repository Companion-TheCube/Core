#pragma once
#include <string>
#include <vector>
#include <functional>
#include <logger.h>
#include <thread>
#ifdef __linux__
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include <boost/best/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

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
    bool setName(std::string name);
    bool setPath(std::string path);
    bool setPublic(bool publicEndpoint);
    void setAction(std::function<void()> action);
    void performAction();
};

class API {
private:
    std::vector<Endpoint*> endpoints;
    CubeLog *logger;
    std::jthread listenerThread;
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
    void listenerThreadFunction();
    void handleRequest(http::request<http::string_body> req, http::response<http::string_body>& res);
    void doSession(tcp::socket& socket);
    void fail(beast::error_code ec, char const* what);
};