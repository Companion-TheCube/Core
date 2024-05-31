#include "messageBox.h"

CubeMessageBox::CubeMessageBox(CubeLog *logger, Shader* shader, Shader* textShader, Renderer* renderer, std::latch& latch)
{
    this->logger = logger;
    this->shader = shader;
    this->textShader = textShader;
    this->visible = false;
    this->latch = &latch;
    this->renderer = renderer;
    this->logger->log("MessageBox initialized", true);
}

CubeMessageBox::~CubeMessageBox()
{
    this->logger->log("MessageBox destroyed", true);
}

void CubeMessageBox::setup()
{
    
    float radius = BOX_RADIUS;
    float diameter = radius * 2;
    float xStart = -0.2;
    float yStart = -0.2;
    glm::vec2 size = { 1.f, 1.f };
    // TODO: make the outline more like a thought box
    this->objects.push_back(new M_Rect(logger, shader, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, { size.x - diameter, size.y - diameter }, 0.0, 0.0)); // main box

    this->objects.push_back(new M_Rect(logger, shader, { xStart, yStart + radius, Z_DISTANCE + this->index }, { radius, size.y - diameter }, 0.0, 0.0)); // left
    this->objects.push_back(new M_Rect(logger, shader, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + this->index }, { radius, size.y - diameter }, 0.0, 0.0)); // right
    this->objects.push_back(new M_Rect(logger, shader, { xStart + radius, yStart, Z_DISTANCE + this->index }, { size.x - diameter, radius }, 0.0, 0.0)); // top
    this->objects.push_back(new M_Rect(logger, shader, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + this->index }, { size.x - diameter, radius }, 0.0, 0.0)); // bottom

    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, { xStart + size.x - radius, yStart + size.y - radius, Z_DISTANCE + this->index }, 0, 90, 0.0)); // top right
    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + this->index }, 90, 180, 0.0)); // top left
    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, 180, 270, 0.0)); // bottom left
    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + this->index }, 270, 360, 0.0)); // bottom right

    this->objects.push_back(new M_Line(logger, shader, { xStart + radius, yStart + size.y, Z_DISTANCE + 0.001 + this->index }, { xStart + size.x - radius, yStart + size.y, Z_DISTANCE + 0.001 + this->index })); // top
    this->objects.push_back(new M_Line(logger, shader, { xStart + size.x, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart + size.x, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // right
    this->objects.push_back(new M_Line(logger, shader, { xStart + radius, yStart, Z_DISTANCE + 0.001 + this->index }, { xStart + size.x - radius, yStart, Z_DISTANCE + 0.001 + this->index })); // bottom
    this->objects.push_back(new M_Line(logger, shader, { xStart, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // left

    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 0, 90, { xStart + size.x - radius, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // top right
    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 360, 270, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom right
    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 180, 270, { xStart + radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom left
    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 180, 90, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // top left

    std::lock_guard<std::mutex> lock(this->mutex);
    this->latch->count_down();
    this->logger->log("MessageBox setup", true);
}

void CubeMessageBox::setText(std::string text)
{
    auto fn = [&, text](){
        this->logger->log("Objects size: " + std::to_string(this->objects.size()), true);
        this->logger->log("TextMeshIndices size: " + std::to_string(this->textMeshIndices.size()), true);
        this->logger->log("Objects size in memory: " + std::to_string(this->objects.size() * sizeof(MeshObject)), true);
        for(size_t index = 0; index < this->textMeshIndices.size(); index++)
        {
            // delete the object referenced by the textMeshIndices
            this->objects[this->textMeshIndices[index]]->~MeshObject();
        }
        // remove the vector entries for the textMeshIndices
        for(size_t index = 0; index < this->textMeshIndices.size(); index++)
        {
            this->objects.erase(this->objects.begin() + this->textMeshIndices[index]);
            // decrement all the indices after the current index
            for(size_t i = index + 1; i < this->textMeshIndices.size(); i++)
            {
                this->textMeshIndices[i]--;
            }
        }
        this->textMeshIndices.clear();
        float xStart = -0.2;
        float yStart = -0.2;
        float textXStart = mapRange(xStart, -1.f, 1.f, 0.f, 720.f);
        float textYStart = mapRange(yStart, -1.f, 1.f, 0.f, 720.f) + mapRange(1.f, 0.f, 2.f, 0.f, 720.f);
        std::vector<std::string> lines;
        std::string line;
        std::istringstream textStream(text);
        while(std::getline(textStream, line))
        {
            lines.push_back(line);
        }
        for(size_t i = 0; i < lines.size(); i++)
        {
            this->objects.push_back(new M_Text(logger, textShader, lines[i], this->messageTextSize, {1.f,1.f,1.f}, { textXStart + STENCIL_INSET_PX, textYStart - STENCIL_INSET_PX - ((i + 1) * this->messageTextSize * (i>0?1.1:1.f))}));
            this->textMeshIndices.push_back(this->objects.size() - 1);
        }
        this->logger->log("MessageBox text set", true);
    };
    this->renderer->addSetupTask(fn);
}


bool CubeMessageBox::setVisible(bool visible)
{
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool CubeMessageBox::getVisible()
{
    return this->visible;
}

void CubeMessageBox::draw()
{
    if(!this->visible)
    {
        return;
    }
    for(auto object : this->objects)
    {
        object->draw();
    }
    
}

void CubeMessageBox::setPosition(glm::vec2 position)
{
    // TODO:
}

void CubeMessageBox::setSize(glm::vec2 size)
{
    // TODO:
}

std::vector<MeshObject*> CubeMessageBox::getObjects()
{
    return this->objects;
}