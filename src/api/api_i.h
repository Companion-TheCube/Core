#pragma once
#ifndef API_I_H
#define API_I_H
#include <expected>
#include <functional>
#include <logger.h>
#include <string>
#include <thread>
#include <vector>
#ifndef HTTPLIB_INCLUDED
#define HTTPLIB_INCLUDED
#include <httplib.h>
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

struct EndpointError {
    enum ERROR_TYPES {
        ENDPOINT_NO_ERROR,
        ENDPOINT_INVALID_REQUEST,
        ENDPOINT_INVALID_PARAMS,
        ENDPOINT_INTERNAL_ERROR,
        ENDPOINT_NOT_IMPLEMENTED,
        ENDPOINT_NOT_AUTHORIZED,
        ENDPOINT_NOT_FOUND,
    };
    EndpointError(ERROR_TYPES errorType, std::string errorString)
        : errorType(errorType)
        , errorString(errorString)
    {
    }
    ERROR_TYPES errorType;
    std::string errorString;
};

typedef std::function<EndpointError(const httplib::Request& req, httplib::Response& res)> EndpointAction_t;
// Type: HttpEndPointDataSinglet_t - A tuple containing the following elements:
// 1. unsigned int - The type of endpoint. This is a bitwise OR of the following values:
//      PUBLIC_ENDPOINT - The endpoint is public and can be accessed by any client on the network.
//      PRIVATE_ENDPOINT - The endpoint is private and can only be accessed by authenticated clients.
//      GET_ENDPOINT - The endpoint is a GET endpoint.
//      POST_ENDPOINT - The endpoint is a POST endpoint.
// 2. EndpointAction_t - The action to be executed when the endpoint is hit. This function should return as soon as possible and should be thread safe.
// 3. std::string - The name of the endpoint.
// 4. std::vector<std::string> - The parameters that the endpoint accepts.
// 5. std::string - A description of the endpoint.
typedef std::tuple<unsigned int, EndpointAction_t, std::string, std::vector<std::string>, std::string> HttpEndPointDataSinglet_t;
typedef std::vector<HttpEndPointDataSinglet_t> HttpEndPointData_t;

#define PUBLIC_ENDPOINT (int)1
#define PRIVATE_ENDPOINT (int)2
#define GET_ENDPOINT (int)4
#define POST_ENDPOINT (int)8

class I_API_Interface {
public:
    virtual ~I_API_Interface() = default;
    virtual std::string getInterfaceName() const = 0;
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
    // virtual std::vector<std::pair<std::string, std::vector<std::string>>> getHttpEndpointNamesAndParams() = 0;
};

#endif