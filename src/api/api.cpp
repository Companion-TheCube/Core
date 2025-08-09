/*
 █████╗ ██████╗ ██╗    ██████╗██████╗ ██████╗ 
██╔══██╗██╔══██╗██║   ██╔════╝██╔══██╗██╔══██╗
███████║██████╔╝██║   ██║     ██████╔╝██████╔╝
██╔══██║██╔═══╝ ██║   ██║     ██╔═══╝ ██╔═══╝ 
██║  ██║██║     ██║██╗╚██████╗██║     ██║     
╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef LOGGER_H
#include <logger.h>
#endif
#ifndef API_H
#include "api.h"
#endif
#ifndef AUTHENTICATION_H
#include "authentication.h"
#include <fstream>
#include <sstream>
#endif

/**
 * @brief Construct a new API::API object. This creates a new CubeAuth object for authentication.
 *
 */
API::API()
{
    // TODO: Since we need to make sure that the entire API is built before letting any clients connect,
    // We should make a http server that will operate on a different port with public access that lets
    // clients know if the API is ready. This will be a simple endpoint that returns a 200 OK if the API
    // is ready and a 503 Service Unavailable if the API is not ready. Client must wait for a 200 OK before
    // attempting to connect to the API on the main port / unix socket.

    // this->endpoints = std::vector<Endpoint*>();
    // TODO: // TESTING AUTHENTICATION //// remove
    std::pair<std::string, std::string> keys = CubeAuth::generateKeyPair();
    CubeLog::info("Public key: " + keys.first);
    CubeLog::info("Private key: " + keys.second);
    std::string myData = "To generate true random integers in C++, you can use the facilities provided by the <random> header, which offers a wide range of random number generation utilities. Here's an example of how you can generate true random integers:";
    std::string encrypted = CubeAuth::encryptData(myData, keys.first);
    CubeLog::info("Encrypted data: " + encrypted);
    std::string decrypted = CubeAuth::decryptData(encrypted, keys.second, myData.length());
    CubeLog::info("Decrypted data: " + decrypted);
    // END TESTING AUTHENTICATION ////
}

/**
 * @brief Destroy the API::API object. This deletes all endpoints and stops the API.
 *
 */
API::~API()
{
    // for (auto endpoint : this->endpoints) {
    //     delete endpoint;
    // }
    this->stop();
}

/**
 * @brief Start the API. Starts the API listener thread and the HTTP server.
 *
 */
void API::start()
{
    // start the API
    CubeLog::info("API starting...");
    this->listenerThread = std::jthread(&API::httpApiThreadFn, this);
}

/**
 * @brief Stop the API. This stops the API listener thread and the HTTP server.
 *
 */
void API::stop()
{
    // stop the API
    CubeLog::info("API stopping...");
    this->listenerThread.request_stop();
    // TODO: add checks to make sure these are valid calls.
    // this->server->stop();
    // this->serverIPC->stop();
    //this->listenerThread.join();
    // delete this->server;
    // this->server = nullptr;
    CubeLog::info("API stopped");
}

/**
 * @brief Restart the API. This stops the API and then starts it again.
 *
 */
void API::restart()
{
    // restart the API
    CubeLog::info("API restarting...");
    this->stop();
    this->start();
}

/**
 * @brief Add an endpoint to the API. Creates a new endpoint object and adds it to the list of endpoints.
 *
 * @param name the name of the endpoint
 * @param path the path of the endpoint
 * @param publicEndpoint whether the endpoint is public or not
 * @param action the action to take when the endpoint is called
 */
void API::addEndpoint(const std::string& name, const std::string& path, int endpointType, EndpointAction_t action)
{
    // add an endpoint
    CubeLog::info("Adding endpoint: " + name + " at " + path);
    auto endpoint = std::make_shared<Endpoint>(endpointType, name, path);
    endpoint->setAction(action);
    this->endpoints.push_back(endpoint);
}

/**
 * @brief Get all endpoints in the API.
 *
 * @return std::vector<Endpoint*> a vector of all endpoints
 */
std::vector<std::shared_ptr<Endpoint>> API::getEndpoints()
{
    // get all endpoints
    return this->endpoints;
}

/**
 * @brief Get an endpoint by name.
 *
 * @param name the name of the endpoint
 * @return Endpoint* the endpoint
 */
std::shared_ptr<Endpoint> API::getEndpointByName(const std::string& name)
{
    // get an endpoint by name
    for (auto endpoint : this->endpoints) {
        if (endpoint->getName() == name) {
            return endpoint;
        }
    }
    return nullptr;
}

/**
 * @brief Remove an endpoint by name.
 *
 * @param name the name of the endpoint
 * @return true if the endpoint was removed, false otherwise
 */
bool API::removeEndpoint(const std::string& name)
{
    // remove an endpoint by name
    for (auto endpoint : this->endpoints) {
        if (endpoint->getName() == name) {
            this->endpoints.erase(std::remove(this->endpoints.begin(), this->endpoints.end(), endpoint), this->endpoints.end());
            return true;
        }
    }
    return false;
}

/**
 * @brief The API listener thread function. This function is called when the API listener thread is started and runs until the thread API listener thread is stopped.
 * Expected to be started as a std::jthread.
 *
 */
void API::httpApiThreadFn()
{
    CubeLog::info("API listener thread starting...");
    try {
        // Allow .env to override default HTTP binding when not set explicitly
        if (this->httpAddress == std::string("0.0.0.0")) {
            this->httpAddress = Config::get("HTTP_ADDRESS", this->httpAddress);
        }
        if (this->httpPort == 55280) {
            std::string portStr = Config::get("HTTP_PORT", "");
            if (!portStr.empty()) {
                try { this->httpPort = std::stoi(portStr); } catch (...) { /* keep default on parse error */ }
            }
        }
        // Use configured binding for HTTP server
        this->server = std::make_unique<CubeHttpServer>(this->httpAddress, this->httpPort);
        // Resolve IPC socket path from centralized config (utils Config)
        std::string ipc = Config::get("IPC_SOCKET_PATH", this->ipcPath);
        if (std::filesystem::exists(ipc)) {
            std::filesystem::remove(ipc);
        }
        // Use resolved IPC socket path
        this->serverIPC = std::make_unique<CubeHttpServer>(ipc, 0);
        for (size_t i = 0; i < this->endpoints.size(); i++) {
            // Public endpoints are accessible by any device on the
            // network. Non public endpoints are only available to devices that have been authenticated. The authentication process is not yet implemented.
            // Non public endpoints are those that perform actions on the Cube such as displaying messages on the screen, changing the brightness, etc.
            // Public endpoints are those that provide information about the Cube such as human presence, temperature, etc.
            // Certain public endpoints should have the option to be secured as well. For example, the endpoint that provides the current state of human
            // presence may be set to private if the user does not want to share that information with others on the network.
            // Local apps have access to the IPC server, which is only available as a unix socket.
            CubeLog::debugSilly("Endpoint type: " + std::to_string(this->endpoints.at(i)->endpointType));
            std::function<void(const httplib::Request&, httplib::Response&)> publicAction = [&, i](const httplib::Request& req, httplib::Response& res) {
                auto returned = this->endpoints.at(i)->doAction(req, res);
                if (returned.errorType == EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR) {
                    CubeLog::debug("Endpoint action returned: " + returned.errorString);
                } else {
                    res.set_content("An error occurred: " + returned.errorString, "text/plain");
                    switch (returned.errorType) {
                    case EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST:
                        res.status = httplib::StatusCode::BadRequest_400;
                        CubeLog::error("Invalid request: " + returned.errorString);
                        break;
                    case EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS:
                        res.status = httplib::StatusCode::BadRequest_400;
                        CubeLog::error("Invalid parameters: " + returned.errorString);
                        break;
                    case EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR:
                        res.status = httplib::StatusCode::InternalServerError_500;
                        CubeLog::error("Internal error: " + returned.errorString);
                        break;
                    case EndpointError::ERROR_TYPES::ENDPOINT_NOT_IMPLEMENTED:
                        res.status = httplib::StatusCode::NotImplemented_501;
                        CubeLog::error("Not implemented: " + returned.errorString);
                        break;
                    case EndpointError::ERROR_TYPES::ENDPOINT_NOT_AUTHORIZED:
                        res.status = httplib::StatusCode::Forbidden_403;
                        CubeLog::error("Not authorized: " + returned.errorString);
                        break;
                    }
                }
            };
            if (this->endpoints.at(i)->isPublic()) {
                CubeLog::debugSilly("Adding public endpoint: " + this->endpoints.at(i)->getName() + " at " + this->endpoints.at(i)->getPath());
                this->server->addEndpoint(this->endpoints.at(i)->isGetType(), this->endpoints[i]->getPath(), publicAction);
            } else {
                CubeLog::debugSilly("Adding non public endpoint: " + this->endpoints.at(i)->getName() + " at " + this->endpoints.at(i)->getPath());
                std::function<void(const httplib::Request&, httplib::Response&)> action = [&, i](const httplib::Request& req, httplib::Response& res) {
                    // first we get the authorization header
                    if (!req.has_header("Authorization")) {
                        res.set_content("Authorization header not present", "text/plain");
                        res.status = httplib::StatusCode::Forbidden_403;
                        return;
                    }
                    std::string authHeader = req.get_header_value("Authorization");
                    // if the authorization header is not present, we return a 403
                    if (authHeader.empty()) {
                        res.set_content("Authorization header not present", "text/plain");
                        res.status = httplib::StatusCode::Forbidden_403;
                        return;
                    }
                    // if the authorization header is present, we check if it is valid
                    if (!CubeAuth::isAuthorized_authHeader(authHeader)) {
                        res.set_content("Authorization header not valid", "text/plain");
                        res.status = httplib::StatusCode::Forbidden_403;
                        return;
                    }
                    // if the authorization header is valid, client is authorized
                    auto returned = this->endpoints.at(i)->doAction(req, res);
                    if (returned.errorType == EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR) {
                        // Do not overwrite the response body on success; endpoint handlers
                        // are responsible for setting res (status/content/type). We only log.
                        CubeLog::debug("Endpoint action returned OK: " + returned.errorString);
                    } else {
                        res.set_content("An error occurred: " + returned.errorString, "text/plain");
                        switch (returned.errorType) {
                        case EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST:
                            res.status = httplib::StatusCode::BadRequest_400;
                            CubeLog::error("Invalid request: " + returned.errorString);
                            break;
                        case EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS:
                            res.status = httplib::StatusCode::BadRequest_400;
                            CubeLog::error("Invalid parameters: " + returned.errorString);
                            break;
                        case EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR:
                            res.status = httplib::StatusCode::InternalServerError_500;
                            CubeLog::error("Internal error: " + returned.errorString);
                            break;
                        case EndpointError::ERROR_TYPES::ENDPOINT_NOT_IMPLEMENTED:
                            res.status = httplib::StatusCode::NotImplemented_501;
                            CubeLog::error("Not implemented: " + returned.errorString);
                            break;
                        case EndpointError::ERROR_TYPES::ENDPOINT_NOT_AUTHORIZED:
                            res.status = httplib::StatusCode::Forbidden_403;
                            CubeLog::error("Not authorized: " + returned.errorString);
                            break;
                        }
                    }
                };
                this->server->addEndpoint(this->endpoints.at(i)->isGetType(), this->endpoints.at(i)->getPath(), action);
            }
            // IPC server is always public. It is only accessible from localhost.
            this->serverIPC->addEndpoint(this->endpoints.at(i)->isGetType(), this->endpoints.at(i)->getPath(), publicAction);
        }
        this->server->start();
        this->serverIPC->start();
        // wait for the stop signal. once this->server goes out of scope, the server will stop.
        while (true) {
            if (this->listenerThread.get_stop_token().stop_requested()) {
                break;
            }
            genericSleep(100);
        }
    } catch (std::exception& e) {
        CubeLog::error(e.what());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new Endpoint::Endpoint object
 *
 * @param endpointType
 * @param name
 * @param path
 */
Endpoint::Endpoint(int endpointType, const std::string& name, const std::string& path)
{
    this->endpointType = endpointType;
    this->name = name;
    this->path = path;
}

/**
 * @brief Destroy the Endpoint::Endpoint object
 *
 */
Endpoint::~Endpoint()
{
    // do nothing
}

/**
 * @brief Get the name of the endpoint
 *
 * @return std::string the name of the endpoint
 */
std::string Endpoint::getName()
{
    return this->name;
}

/**
 * @brief Get the path of the endpoint
 *
 * @return std::string the path of the endpoint
 */
std::string Endpoint::getPath()
{
    return this->path;
}

/**
 * @brief Check if the endpoint is public
 *
 * @return true if the endpoint is public, false otherwise
 */
bool Endpoint::isPublic() const
{
    return (this->endpointType & PUBLIC_ENDPOINT) == PUBLIC_ENDPOINT;
}

/**
 * @brief check if the endpoint is a GET endpoint
 *
 * @return true if the endpoint is a GET endpoint, false otherwise
 */
bool Endpoint::isGetType() const
{
    return (this->endpointType & GET_ENDPOINT) == GET_ENDPOINT;
}

/**
 * @brief Set the action to take when the endpoint is called
 *
 * @param action the action to take
 */
void Endpoint::setAction(EndpointAction_t action)
{
    this->action = action;
}

/**
 * @brief Perform the action when the endpoint is called
 *
 * @param req the request object
 * @param res the response object
 * @return std::string the response to send back to the client
 */
EndpointError Endpoint::doAction(const httplib::Request& req, httplib::Response& res)
{
    return this->action(req, res);
}

/**
 * @brief Get the action to take when the endpoint is called
 *
 * @return std::function<std::string(std::string, EndPointParams_t)> the action to take
 */
EndpointAction_t Endpoint::getAction()
{
    return this->action;
}

////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new CubeHttpServer::CubeHttpServer object
 *
 * @param address the address to bind the server to
 * @param port the port to bind the server to. 0 for unix sockets.
 */
CubeHttpServer::CubeHttpServer(const std::string& address, int port)
{
    this->address = address;
    this->port = port;
    // check to see if the SSL files exist
    if(std::filesystem::exists("ssl/server.crt") && std::filesystem::exists("ssl/server.key")) {
        CubeLog::info("SSL files found, enabling SSL support for HTTP server");
        this->server = std::make_shared<httplib::SSLServer>("ssl/server.crt", "ssl/server.key");
        if (!this->server->is_valid()) {
            CubeLog::error("Failed to create SSL server. Falling back to non-SSL server.");
            this->server = std::make_shared<httplib::Server>();
        } else {
            CubeLog::info("SSL server created successfully.");
        }
    } else {
        CubeLog::info("SSL files not found, starting non-SSL HTTP server");
        this->server = std::make_shared<httplib::Server>();
    }
}

/**
 * @brief Destroy the CubeHttpServer::CubeHttpServer object. This deletes the server object.
 *
 */
CubeHttpServer::~CubeHttpServer()
{
    // delete this->server;
    this->stop();
}

/**
 * @brief Start the server. This binds the server to the address and port and starts listening for requests.
 *
 */
void CubeHttpServer::start()
{
    // start the server
    CubeLog::info("HTTP server starting...");
    if (this->port > 0) {
        if (!this->server->bind_to_port(this->address.c_str(), this->port)) {
            CubeLog::error("Failed to bind HTTP server to " + this->address + ":" + std::to_string(this->port));
            return;
        }
    }
    CubeLog::debugSilly("HTTP server bound to " + this->address + ":" + std::to_string(this->port));
    this->server->set_logger([&](const httplib::Request& req, const httplib::Response& res) {
        CubeLog::info("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status));
    });
    CubeLog::debugSilly("HTTP server set logger");
    this->server->set_error_handler([&](const httplib::Request& req, httplib::Response& res) {
        CubeLog::error("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status));
    });
    CubeLog::debugSilly("HTTP server set error handler");
    serverThread = std::make_unique<std::jthread>([&] {
        if (this->port > 0)
            this->server->listen_after_bind();
        else
            this->server->set_address_family(AF_UNIX).listen(this->address, 80);
    });
    this->server->wait_until_ready();
    CubeLog::debugSilly(this->server->is_running() ? "HTTP server is running" : "HTTP server is not running");
}

/**
 * @brief Stop the server. This stops the server from listening for requests.
 *
 */
void CubeHttpServer::stop()
{
    CubeLog::info("HTTP server stopping...");
    this->server->stop();
    genericSleep(250);
    // Workaround for httplib with AF_UNIX: if bound to a UNIX socket and no client
    // ever connected, listen thread may hang during stop(). Connect a dummy client.
    if (this->port == 0 && !this->address.empty()) {
        int dummy_fd = socket(AF_UNIX, SOCK_STREAM, 0);
#ifdef __linux__
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, this->address.c_str(), sizeof(addr.sun_path) - 1);
        connect(dummy_fd, (struct sockaddr*)&addr, sizeof(addr));
        close(dummy_fd);
#endif
    }
    while (this->server->is_running()) {
        genericSleep(100);
    }
    this->server.reset();
    serverThread->request_stop();
    serverThread->join();
}

/**
 * @brief Restart the server. This stops the server and then starts it again.
 *
 */
void CubeHttpServer::restart()
{
    // restart the server
    CubeLog::info("HTTP server restarting...");
    this->stop();
    this->server = std::make_shared<httplib::Server>();
    this->start();
}

/**
 * @brief Add an endpoint to the server.
 *
 * @param path the path of the endpoint
 * @param action the action to take when the endpoint is called
 */
void CubeHttpServer::addEndpoint(bool isGetType, const std::string& path, std::function<void(const httplib::Request&, httplib::Response&)> action)
{
    // add an endpoint
    if (isGetType) {
        this->server->Get(path.c_str(), action);
        CubeLog::debug("Added GET endpoint: " + path);
    } else {
        this->server->Post(path.c_str(), action);
        CubeLog::debug("Added POST endpoint: " + path);
    }
}

/**
 * @brief Remove an endpoint from the server.
 *
 * @param path the path of the endpoint
 */
void CubeHttpServer::removeEndpoint(const std::string& path)
{
    // remove an endpoint
    auto action = [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("Endpoint not found", "text/plain");
        res.status = 404;
    };
    this->server->Delete(path.c_str(), action);
}

/**
 * @brief Set the port to bind the server to.
 *
 * @param port the port to bind the server to
 */
void CubeHttpServer::setPort(int port)
{
    this->port = port;
    this->restart();
}

/**
 * @brief Get the port the server is bound to.
 *
 * @return int the port the server is bound to
 */
int CubeHttpServer::getPort()
{
    return this->port;
}

/**
 * @brief Get the server object.
 *
 * @return httplib::Server* the server object
 */
std::shared_ptr<httplib::Server> CubeHttpServer::getServer()
{
    return this->server;
}
