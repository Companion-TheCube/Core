#include "api.h"

API::API(CubeLog *logger){
    this->logger = logger;
    this->endpoints = std::vector<Endpoint*>();
    this->start();
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
    this->listenerThread = std::jthread(&API::listenerThreadFunction, this);
}

void API::stop(){
    // stop the API
    this->logger->log("API stopping...", true);
    this->listenerThread.request_stop();
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

void API::listenerThreadFunction(){
    // the listener thread function
    this->logger->log("API listener thread starting...", true);
    try{
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {tcp::v4(), 55280}};
        while(true){
            if(this->listenerThread.get_stop_token().stop_requested()){
                break;
            }
            // do something
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread{std::bind(&API::doSession, this, std::move(socket))}.detach();
        
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

void API::doSession(tcp::socket& socket){
    // do a session
    try{
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);
        http::response<http::string_body> res;
        this->handleRequest(req, res);
        http::write(socket, res);
    }catch(std::exception &e){
        this->logger->error(e.what());
    }
}

void API::handleRequest(http::request<http::string_body> req, http::response<http::string_body>& res){
    // handle a request
    res.version(req.version());
    res.result(http::status::ok);
    res.set(http::field::server, "Beast");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    res.body() = "Hello, world!";
    res.prepare_payload();
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

bool Endpoint::setName(std::string name){
    // if name contains a space, return false
    if(name.find(" ") != std::string::npos){
        return false;
    }
    this->name = name;
    return true;
}

bool Endpoint::setPath(std::string path){
    // if path contains a space, return false
    if(path.find(" ") != std::string::npos){
        return false;
    }
    this->path = path;
    return true;
}

bool Endpoint::setPublic(bool publicEndpoint){
    bool temp = this->publicEndpoint;
    this->publicEndpoint = publicEndpoint;
    return temp;
}

void Endpoint::setAction(std::function<void()> action){
    this->action = action;
}

void Endpoint::performAction(){
    this->action();
}

////////////////////////////////////////////////////////////////////////////////////////////