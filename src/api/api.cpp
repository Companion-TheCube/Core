#include "api.h"

API::API(){
    this->endpoints = std::vector<Endpoint*>();
}

API::~API(){
    for(auto endpoint : this->endpoints){
        delete endpoint;
    }
    this->stop();
}

void API::start(){
    // start the API
    CubeLog::log("API starting...", true);
    this->listenerThread = std::jthread(&API::httpApiThreadFn, this);
}

void API::stop(){
    // stop the API
    CubeLog::log("API stopping...", true);
    this->listenerThread.request_stop();
    this->server->stop();
    this->listenerThread.join();
}

void API::restart(){
    // restart the API
    CubeLog::log("API restarting...", true);
    this->stop();
    this->start();
}

void API::addEndpoint(std::string name, std::string path, bool publicEndpoint, std::function<std::string(std::string response, std::vector<std::pair<std::string, std::string>> params)> action){
    // add an endpoint
    CubeLog::log("Adding endpoint: " + name + " at " + path, true);
    Endpoint *endpoint = new Endpoint(publicEndpoint, name, path);
    endpoint->setAction(action);
    this->endpoints.push_back(endpoint);
}

std::vector<Endpoint*> API::getEndpoints(){
    // get all endpoints
    return this->endpoints;
}

Endpoint* API::getEndpointByName(std::string name){
    // get an endpoint by name
    for(auto endpoint : this->endpoints){
        if(endpoint->getName() == name){
            return endpoint;
        }
    }
    return nullptr;
}

bool API::removeEndpoint(std::string name){
    // remove an endpoint by name
    for(auto endpoint : this->endpoints){
        if(endpoint->getName() == name){
            this->endpoints.erase(std::remove(this->endpoints.begin(), this->endpoints.end(), endpoint), this->endpoints.end());
            delete endpoint;
            return true;
        }
    }
    return false;
}

void API::httpApiThreadFn(){
    CubeLog::log("API listener thread starting...", true);
    try{
        this->server = new CubeHttpServer("0.0.0.0", 55280);
        // TODO: set up authentication
        for(size_t i = 0; i < this->endpoints.size(); i++){
            if(this->endpoints.at(i)->isPublic()){
                CubeLog::log("Adding public endpoint: " + this->endpoints.at(i)->getName() + " at " + this->endpoints.at(i)->getPath(), true);
                this->server->addEndpoint(this->endpoints[i]->getPath(), [&, i](const httplib::Request &req, httplib::Response &res){
                    std::string response;
                    response += "Endpoint: " + this->endpoints.at(i)->getName() + "\n\n";
                    response += "Method: " + req.method + "\n";
                    response += "Path: " + req.path + "\n";
                    response += "Body: " + req.body + "\n";
                    response += "Params: \n";
                    std::vector<std::pair<std::string, std::string>> params;
                    for(auto param : req.params){
                        response += param.first + ": " + param.second + "\n";
                        params.push_back({param.first, param.second});
                    }
                    res.set_content(response, "text/plain");
                    std::string returned = this->endpoints.at(i)->doAction(response, params);
                    CubeLog::log("Endpoint action returned: " + returned, true);
                });
            }else{
                this->server->addEndpoint(this->endpoints.at(i)->getPath(), [&](const httplib::Request &req, httplib::Response &res){
                    res.set_content("Endpoint not public", "text/plain");
                    res.status = httplib::StatusCode::Forbidden_403;
                });
                // TODO: add non public endpoints
            }
        }
        this->server->start();
        while(true){
            if(this->listenerThread.get_stop_token().stop_requested()){
                break;
            }
            // do something
        
            #ifdef _WIN32
            Sleep(1);
            #endif
            #ifdef __linux__
            usleep(1);
            #endif  
        }
    }catch(std::exception &e){
        CubeLog::error(e.what());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

Endpoint::Endpoint(bool publicEndpoint, std::string name, std::string path){
    this->publicEndpoint = publicEndpoint;
    this->name = name;
    this->path = path;
}

Endpoint::~Endpoint(){
}

std::string Endpoint::getName(){
    return this->name;
}

std::string Endpoint::getPath(){
    return this->path;
}

bool Endpoint::isPublic(){
    return this->publicEndpoint;
}

void Endpoint::setAction(std::function<std::string(std::string response, EndPointParams_t params)> action){
    this->action = action;
}

std::string Endpoint::doAction(std::string response, EndPointParams_t params){
    return this->action(response, params);
}

std::function <std::string(std::string response, EndPointParams_t params)> Endpoint::getAction(){
    return this->action;
}

////////////////////////////////////////////////////////////////////////////////////////////

CubeHttpServer::CubeHttpServer(std::string address, int port){
    this->address = address;
    this->port = port;
    this->server = new httplib::Server();
}

CubeHttpServer::~CubeHttpServer(){
    delete this->server;
}

void CubeHttpServer::start(){
    // start the server
    CubeLog::log("HTTP server starting...", true);
    this->server->bind_to_port(this->address.c_str(), this->port);
    this->server->set_logger([&](const httplib::Request &req, const httplib::Response &res){
        CubeLog::log("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status), true);
    });
    this->server->set_error_handler([&](const httplib::Request &req, httplib::Response &res){
        CubeLog::error("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status));
    });
    this->server->listen_after_bind();
}

void CubeHttpServer::stop(){
    // stop the server
    CubeLog::log("HTTP server stopping...", true);
    this->server->stop();
}

void CubeHttpServer::restart(){
    // restart the server
    CubeLog::log("HTTP server restarting...", true);
    this->stop();
    this->start();
}

void CubeHttpServer::addEndpoint(std::string path, std::function<void(const httplib::Request&, httplib::Response&)> action){
    // add an endpoint
    this->server->Get(path.c_str(), action);
}

void CubeHttpServer::removeEndpoint(std::string path){
    // remove an endpoint
    auto action = [](const httplib::Request &req, httplib::Response &res){
        res.set_content("Endpoint not found", "text/plain");
        res.status = 404;
    };
    this->server->Delete(path.c_str(), action);
}

void CubeHttpServer::setPort(int port){
    this->port = port;
    this->restart();
}

int CubeHttpServer::getPort(){
    return this->port;
}

httplib::Server* CubeHttpServer::getServer(){
    return this->server;
}