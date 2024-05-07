// Path: src/gui/renderer.h
#include "renderer.h"

Renderer::Renderer(CubeLog* logger)
{
    this->t = std::thread(&Renderer::thread, this);
    this->logger = logger;
}

Renderer::~Renderer()
{
}

sf::Vector2f Renderer::project(Vertex& v)
{
    float fov = 256; // Field of View
    float viewer_distance = 4;

    // Simple perspective projection
    float factor = fov / (viewer_distance + v.z);
    float x = v.x * factor + 720 / 2;
    float y = v.y * factor + 720 / 2;
    return sf::Vector2f(x, y);
}

std::vector<Vertex> Renderer::rotateY(float angle, std::vector<Vertex> cubeVertices)
{
    float s = std::sin(angle);
    float c = std::cos(angle);

    for (int i = 0; i < 8; ++i) { // Use loop with known bounds
        float xnew = cubeVertices[i].x * c - cubeVertices[i].z * s;
        float znew = cubeVertices[i].x * s + cubeVertices[i].z * c;
        cubeVertices[i].x = xnew;
        cubeVertices[i].z = znew;
    }
    return cubeVertices;
}

std::vector<Vertex> Renderer::rotateX(float angle, std::vector<Vertex> cubeVertices)
{
    float s = std::sin(angle);
    float c = std::cos(angle);

    for (int i = 0; i < 8; ++i) { // Use loop with known bounds
        float ynew = cubeVertices[i].y * c - cubeVertices[i].z * s;
        float znew = cubeVertices[i].y * s + cubeVertices[i].z * c;
        cubeVertices[i].y = ynew;
        cubeVertices[i].z = znew;
    }
    return cubeVertices;
}

std::vector<Vertex> Renderer::rotateZ(float angle, std::vector<Vertex> cubeVertices)
{
    float s = std::sin(angle);
    float c = std::cos(angle);

    for (int i = 0; i < 8; ++i) { // Use loop with known bounds
        float xnew = cubeVertices[i].x * c - cubeVertices[i].y * s;
        float ynew = cubeVertices[i].x * s + cubeVertices[i].y * c;
        cubeVertices[i].x = xnew;
        cubeVertices[i].y = ynew;
    }
    return cubeVertices;
}


int Renderer::thread()
{

    std::vector<Vertex> cubeVertices = {
        { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 }, // Front face
        { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 } // Back face
    };

    this->window.create(sf::VideoMode(720, 720), "TheCube", sf::Style::Fullscreen);
    bool visibleMouse = false;
    this->window.setFramerateLimit(60);
    this->window.setMouseCursorVisible(visibleMouse);
    int textSize = 24;
    if (!this->font.loadFromFile("/home/cube/.local/share/fonts/Hack-Regular.ttf")) {
        this->logger->error("Could not load font");
    }
    this->logger->log("Renderer initialized. Starting Window...", true);
    while (this->window.isOpen()) {
        for (auto event = sf::Event {}; this->window.pollEvent(event);) {
            if (event.type == sf::Event::Closed) {
                this->window.close();
            }
        }
        this->window.clear(sf::Color::Black);
        cubeVertices = this->rotateY(0.01f, cubeVertices);
        cubeVertices = this->rotateX(0.023f, cubeVertices);
        cubeVertices = this->rotateZ(0.037f, cubeVertices);
        auto drawFace = [&](int a, int b, int c, int d) -> void {
            
            
            sf::Vertex line[] = {
                sf::Vertex(project(cubeVertices[a]), sf::Color::White),
                sf::Vertex(project(cubeVertices[b]), sf::Color::White),
                sf::Vertex(project(cubeVertices[c]), sf::Color::White),
                sf::Vertex(project(cubeVertices[d]), sf::Color::White),
                sf::Vertex(project(cubeVertices[a]), sf::Color::White)
            };
            this->window.draw(line, 5, sf::LineStrip);
        };
        drawFace(0, 1, 2, 3);
        drawFace(4, 5, 6, 7);
        drawFace(1, 5, 6, 2);
        drawFace(0, 4, 7, 3);
        drawFace(3, 2, 6, 7);
        drawFace(0, 1, 5, 4);

        sf::VertexArray triangle(sf::Triangles, 3);

        // define the position of the triangle's points
        triangle[0].position = sf::Vector2f(100.f, 100.f);
        triangle[1].position = sf::Vector2f(200.f, 100.f);
        triangle[2].position = sf::Vector2f(200.f, 200.f);

        // define the color of the triangle's points
        triangle[0].color = sf::Color::Red;
        triangle[1].color = sf::Color::Blue;
        triangle[2].color = sf::Color::Green;
        this->window.draw(triangle);

        sf::Text text;

        // select the font
        text.setFont(this->font); // font is a sf::Font

        // set the string to display
        text.setString("Hello world");

        // set the character size
        text.setCharacterSize(textSize++); // in pixels, not points!
        if (textSize > 50)
            textSize = 1;

        // set the color
        text.setFillColor(sf::Color::Red);

        // set the text style
        text.setStyle(sf::Text::Bold | sf::Text::Underlined);

        // inside the main loop, between window.clear() and window.display()
        this->window.draw(text);

        // window.clear();
        this->window.display();
    }
    return 0;
}