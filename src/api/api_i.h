#pragma once
#include <string>
#include <vector>
#include <functional>
#include <logger.h>
#include <thread>

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