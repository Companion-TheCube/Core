#pragma once
#ifndef API_I_H
#define API_I_H
#include <expected>
#include <functional>
#include <logger.h>
#include <string>
#include <thread>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <httplib.h>
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
typedef std::pair<unsigned int, EndpointAction_t> HttpEndPointDataSinglet_t;
typedef std::vector<std::pair<unsigned int, EndpointAction_t>> HttpEndPointData_t; // TODO: change this to a tuple with the endpoint name/path and params

#define PUBLIC_ENDPOINT (int)1
#define PRIVATE_ENDPOINT (int)2
#define GET_ENDPOINT (int)4
#define POST_ENDPOINT (int)8

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
    virtual HttpEndPointData_t getHttpEndpointData() = 0;
    virtual std::vector<std::pair<std::string, std::vector<std::string>>> getHttpEndpointNamesAndParams() = 0;
};

#endif