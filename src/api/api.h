#ifndef API_H
#define API_H
#include <any>
#include <filesystem>
#include <functional>
#include <iostream>
#include <latch>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#ifndef HTTPLIB_INCLUDED
#define HTTPLIB_INCLUDED
#define CPPHTTPLIB_OPENSSL_SUPPORT
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
#include <nlohmann/json.hpp>
#include <utils.h>

#define CUBE_SOCKET_PATH "cube.sock"
#define PUBLIC_ENDPOINT (int)1
#define PRIVATE_ENDPOINT (int)2
#define GET_ENDPOINT (int)4
#define POST_ENDPOINT (int)8

struct EndpointError {
    enum class ERROR_TYPES {
        ENDPOINT_NO_ERROR,
        ENDPOINT_INVALID_REQUEST,
        ENDPOINT_INVALID_PARAMS,
        ENDPOINT_INTERNAL_ERROR,
        ENDPOINT_NOT_IMPLEMENTED,
        ENDPOINT_NOT_AUTHORIZED,
        ENDPOINT_NOT_FOUND,
    };
    EndpointError(ERROR_TYPES errorType, const std::string& errorString)
        : errorType(errorType)
        , errorString(errorString)
    {
    }
    ERROR_TYPES errorType;
    std::string errorString;
};

typedef std::function<EndpointError(const httplib::Request& req, httplib::Response& res)> EndpointAction_t;
typedef std::tuple<unsigned int, EndpointAction_t, std::string, std::vector<std::string>, std::string> HttpEndPointDataSinglet_t;
typedef std::vector<HttpEndPointDataSinglet_t> HttpEndPointData_t;

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

class I_API_Interface {
public:
    virtual ~I_API_Interface() = default;
    constexpr virtual std::string getInterfaceName() const = 0;
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
    virtual HttpEndPointData_t getHttpEndpointData() = 0;
};

class API_Builder {
private:
    static std::shared_ptr<API> api;
    static std::unordered_map<std::string, std::shared_ptr<I_API_Interface>> interface_objs;

public:
    // Pass in all the dependencies the API needs
    API_Builder(std::shared_ptr<API> api);
    ~API_Builder();
    void start();
    template <typename T>
    static void addInterfaceStatic(std::shared_ptr<T> interface_object)
    {
        // CubeLog::info("Adding interface object: " + interface_object->getInterfaceName());
        try {
            auto interface_name = interface_object->getInterfaceName();
            interface_objs[interface_name] = interface_object;
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        // TODO: We need to make the API server is able to handle new interfaces being added at runtime.
    }
    template <typename T>
    void addInterface(std::shared_ptr<T> interface_object)
    {
        // CubeLog::info("Adding interface object: " + interface_object->getInterfaceName());
        interface_objs[interface_object->getInterfaceName()] = interface_object;
    }
};

/*

This class template is used to automatically register an API interface with the API_Builder class.
Classes that implements I_API_Interface should inherit from this class template as well.
This automatically registers the interface with the API_Builder class when the class is instantiated.

When the builder class has its start() method called, it will wait until all the interfaces have been
added to the builder before it starts building the API. This is to ensure that all the interfaces are
added before the API is built.

To make this work, the CMakeLists file defines a function that counts all the references to the
AutoRegisterAPI class template in the code every time we compile. This count is then used to
determine how many interfaces are expected to be added to the API. the function that does this gets
written to a cmake file that gets included in the CMakeLists file. It then writes the define
into InterfaceCount.h. This file must be included in builder.cpp.

*/
template <typename T>
class AutoRegisterAPI : public I_API_Interface, public std::enable_shared_from_this<T> {
public:
    AutoRegisterAPI()
    {
        static_assert(std::is_base_of<AutoRegisterAPI<T>, T>::value,
            "T must inherit from AutoRegisterAPI<T>");
    }
    void registerInterface()
    {
        if (this->___api_registered)
            return;
        // Ensure the object is managed by a shared_ptr
        auto self = std::dynamic_pointer_cast<T>(this->shared_from_this());
        if (!self) {
            throw std::runtime_error("This instance must be created using std::make_shared or std::shared_ptr.");
        }

        // Register the shared_ptr with the API builder
        API_Builder::addInterfaceStatic(self);
        this->___api_registered = true;
    }

private:
    bool ___api_registered = false;
};

#endif // API_H