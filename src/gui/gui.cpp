#include "./gui.h"

GUI::GUI(CubeLog *logger)
{
    this->logger = logger;
    this->renderer = new Renderer(this->logger);
    this->logger->log("GUI initialized", true);
}

GUI::~GUI()
{
    delete this->renderer;
    this->logger->log("GUI destroyed", true);
}