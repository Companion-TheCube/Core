// TODO: Interprocess communication between the the CubeCore and the "Apps" which will facilitated through a library that the Apps can use.

// TODO: HTTP API needs an endpoint that lists all endpoints

#include "api.h"

/**
 * @brief Construct a new API::API object. This creates a new CubeAuth object for authentication.
 *
 */
API::API()
{
    this->endpoints = std::vector<Endpoint*>();
    this->auth = new CubeAuth();
    // TODO: // TESTING AUTHENTICATION ////
    std::pair<std::string, std::string> keys = auth->generateKeyPair();
    CubeLog::info("Public key: " + keys.first);
    CubeLog::info("Private key: " + keys.second);
    std::string myData = "To generate true random integers in C++, you can use the facilities provided by the <random> header, which offers a wide range of random number generation utilities. Here's an example of how you can generate true random integers:";
    std::string encrypted = auth->encryptData(myData, keys.first);
    CubeLog::info("Encrypted data: " + encrypted);
    std::string decrypted = auth->decryptData(encrypted, keys.second, myData.length());
    CubeLog::info("Decrypted data: " + decrypted);
    //// END TESTING AUTHENTICATION ////
}

/**
 * @brief Destroy the API::API object. This deletes all endpoints and stops the API.
 *
 */
API::~API()
{
    for (auto endpoint : this->endpoints) {
        delete endpoint;
    }
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
    this->server->stop();
    this->listenerThread.join();
    delete this->server;
    this->server = nullptr;
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
void API::addEndpoint(std::string name, std::string path, int endpointType, std::function<std::string(const httplib::Request& req, httplib::Response& res)> action)
{
    // add an endpoint
    CubeLog::info("Adding endpoint: " + name + " at " + path);
    Endpoint* endpoint = new Endpoint(endpointType, name, path);
    endpoint->setAction(action);
    this->endpoints.push_back(endpoint);
}

/**
 * @brief Get all endpoints in the API.
 *
 * @return std::vector<Endpoint*> a vector of all endpoints
 */
std::vector<Endpoint*> API::getEndpoints()
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
Endpoint* API::getEndpointByName(std::string name)
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
bool API::removeEndpoint(std::string name)
{
    // remove an endpoint by name
    for (auto endpoint : this->endpoints) {
        if (endpoint->getName() == name) {
            this->endpoints.erase(std::remove(this->endpoints.begin(), this->endpoints.end(), endpoint), this->endpoints.end());
            delete endpoint;
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
        this->server = new CubeHttpServer("0.0.0.0", 55280);
        // TODO: set up authentication
        for (size_t i = 0; i < this->endpoints.size(); i++) {
            // TODO: need to document the difference between public and non public endpoints. Public endpoints are accessible by any device on the
            // network. Non public endpoints are only available to devices that have been authenticated. The authentication process is not yet implemented.
            // Non public endpoints are those that perform actions on the Cube such as displaying messages on the screen, changing the brightness, etc.
            // Public endpoints are those that provide information about the Cube such as human presence, temperature, etc.
            // Certain public endpoints should have the option to be secured as well. For example, the endpoint that provides the current state of human
            // presence may be set to private if the user does not want to share that information with others on the network.
            CubeLog::debug("Endpoint type: " + std::to_string(this->endpoints.at(i)->endpointType));
            if (this->endpoints.at(i)->isPublic()) {
                CubeLog::info("Adding public endpoint: " + this->endpoints.at(i)->getName() + " at " + this->endpoints.at(i)->getPath());
                this->server->addEndpoint(this->endpoints.at(i)->isGetType(), this->endpoints[i]->getPath(), [&, i](const httplib::Request& req, httplib::Response& res) {
                    std::string returned = this->endpoints.at(i)->doAction(req, res);
                    if (returned != "")
                        res.set_content(returned, "text/plain");
                    CubeLog::info("Endpoint action returned: " + (returned == "" ? "empty string" : returned));
                });
            } else {
                CubeLog::info("Adding non public endpoint: " + this->endpoints.at(i)->getName() + " at " + this->endpoints.at(i)->getPath());
                this->server->addEndpoint(this->endpoints.at(i)->isGetType(), this->endpoints.at(i)->getPath(), [&, i](const httplib::Request& req, httplib::Response& res) {
                    // res.set_content("Endpoint not public", "text/plain");
                    // res.status = httplib::StatusCode::Forbidden_403;
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
                    if (!this->auth->isAuthorized_authHeader(authHeader)) {
                        res.set_content("Authorization header not valid", "text/plain");
                        res.status = httplib::StatusCode::Forbidden_403;
                        return;
                    }
                    // if the authorization header is valid, client is authorized
                    std::string returned = this->endpoints.at(i)->doAction(req, res);
                    if (returned != "")
                        res.set_content(returned, "text/plain");
                    CubeLog::info("Endpoint action returned: " + (returned == "" ? "empty string" : returned));
                });
            }
        }
        this->server->start();
        // wait for the stop signal. once this->server goes out of scope, the server will stop.
        while (true) {
            if (this->listenerThread.get_stop_token().stop_requested()) {
                break;
            }
            genericSleep(10);
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
Endpoint::Endpoint(int endpointType, std::string name, std::string path)
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
void Endpoint::setAction(std::function<std::string(const httplib::Request& req, httplib::Response& res)> action)
{
    this->action = action;
}

/**
 * @brief Perform the action when the endpoint is called // TODO: response and params parameters should be combined into a single object
 *
 * @param response the response from the client
 * @param params the parameters from the client
 * @return std::string the response to send back to the client
 */
std::string Endpoint::doAction(const httplib::Request& req, httplib::Response& res)
{
    return this->action(req, res);
}

/**
 * @brief Get the action to take when the endpoint is called
 *
 * @return std::function<std::string(std::string, EndPointParams_t)> the action to take
 */
std::function<std::string(const httplib::Request& req, httplib::Response& res)> Endpoint::getAction()
{
    return this->action;
}

////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new CubeHttpServer::CubeHttpServer object
 *
 * @param address the address to bind the server to
 * @param port the port to bind the server to
 */
CubeHttpServer::CubeHttpServer(std::string address, int port)
{
    this->address = address;
    this->port = port;
    this->server = new httplib::Server();
}

/**
 * @brief Destroy the CubeHttpServer::CubeHttpServer object. This deletes the server object.
 *
 */
CubeHttpServer::~CubeHttpServer()
{
    delete this->server;
}

/**
 * @brief Start the server. This binds the server to the address and port and starts listening for requests.
 *
 */
void CubeHttpServer::start()
{
    // start the server
    CubeLog::info("HTTP server starting...");
    this->server->bind_to_port(this->address.c_str(), this->port);
    this->server->set_logger([&](const httplib::Request& req, const httplib::Response& res) {
        CubeLog::info("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status));
    });
    this->server->set_error_handler([&](const httplib::Request& req, httplib::Response& res) {
        CubeLog::error("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status));
    });
    this->server->listen_after_bind();
}

/**
 * @brief Stop the server. This stops the server from listening for requests.
 *
 */
void CubeHttpServer::stop()
{
    // stop the server
    CubeLog::info("HTTP server stopping...");
    this->server->stop();
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
    this->start();
}

/**
 * @brief Add an endpoint to the server.
 *
 * @param path the path of the endpoint
 * @param action the action to take when the endpoint is called
 */
void CubeHttpServer::addEndpoint(bool isGetType, std::string path, std::function<void(const httplib::Request&, httplib::Response&)> action)
{
    // add an endpoint
    if (isGetType) {
        this->server->Get(path.c_str(), action);
        CubeLog::info("Added GET endpoint: " + path);
    } else {
        this->server->Post(path.c_str(), action);
        CubeLog::info("Added POST endpoint: " + path);
    }
}

/**
 * @brief Remove an endpoint from the server.
 *
 * @param path the path of the endpoint
 */
void CubeHttpServer::removeEndpoint(std::string path)
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
httplib::Server* CubeHttpServer::getServer()
{
    return this->server;
}