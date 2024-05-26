#include "api.h"

API::API(CubeLog *logger){
    this->logger = logger;
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
    this->logger->log("API starting...", true);
    this->listenerThread = std::jthread(&API::httpApiThreadFn, this);
}

void API::stop(){
    // stop the API
    this->logger->log("API stopping...", true);
    this->listenerThread.request_stop();
    this->server->stop();
    this->listenerThread.join();
}

void API::restart(){
    // restart the API
    this->logger->log("API restarting...", true);
    this->stop();
    this->start();
}

void API::addEndpoint(std::string name, std::string path, bool publicEndpoint, std::function<void()> action){
    // add an endpoint
    this->logger->log("Adding endpoint: " + name + " at " + path, true);
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
    this->logger->log("API listener thread starting...", true);
    try{
        this->server = new CubeHttpServer(this->logger, "0.0.0.0", 55280);
        for(size_t i = 0; i < this->endpoints.size(); i++){
            if(this->endpoints.at(i)->isPublic()){
                this->logger->log("Adding public endpoint: " + this->endpoints.at(i)->getName() + " at " + this->endpoints.at(i)->getPath(), true);
                this->server->addEndpoint(this->endpoints[i]->getPath(), [&, i](const httplib::Request &req, httplib::Response &res){
                    std::string response;
                    response += "Endpoint: " + this->endpoints.at(i)->getName() + "\n\n";
                    for(auto param : req.params){
                        response += param.first + ": " + param.second + "\n";
                    }
                    res.set_content(response, "text/plain");
                    this->endpoints.at(i)->doAction();
                });
            }else{
                this->server->addEndpoint(this->endpoints.at(i)->getPath(), [&](const httplib::Request &req, httplib::Response &res){
                    res.set_content("Endpoint not public", "text/plain");
                    res.status = 403;
                });
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
        this->logger->error(e.what());
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

void Endpoint::setAction(std::function<void()> action){
    this->action = action;
}

void Endpoint::doAction(){
    this->action();
}

std::function <void()> Endpoint::getAction(){
    return this->action;
}

////////////////////////////////////////////////////////////////////////////////////////////

CubeHttpServer::CubeHttpServer(CubeLog *logger, std::string address, int port){
    this->logger = logger;
    this->address = address;
    this->port = port;
    this->server = new httplib::Server();
}

CubeHttpServer::~CubeHttpServer(){
    delete this->server;
}

void CubeHttpServer::start(){
    // start the server
    this->logger->log("HTTP server starting...", true);
    this->server->bind_to_port(this->address.c_str(), this->port);
    this->server->set_logger([&](const httplib::Request &req, const httplib::Response &res){
        this->logger->log("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status), true);
    });
    this->server->set_error_handler([&](const httplib::Request &req, httplib::Response &res){
        this->logger->error("HTTP server: " + req.method + " " + req.path + " " + std::to_string(res.status));
    });
    this->server->listen_after_bind();
}

void CubeHttpServer::stop(){
    // stop the server
    this->logger->log("HTTP server stopping...", true);
    this->server->stop();
}

void CubeHttpServer::restart(){
    // restart the server
    this->logger->log("HTTP server restarting...", true);
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