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
    float fov = 720; // Field of View
    float viewer_distance = 5;

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

    this->window.create(sf::VideoMode(720, 720), "TheCube", sf::Style::None);
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
        cubeVertices = this->rotateY(0.001f, cubeVertices);
        cubeVertices = this->rotateX(0.007f, cubeVertices);
        cubeVertices = this->rotateZ(0.017f, cubeVertices);
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
        auto drawEdge = [&](int a, int b) -> void {
            sf::Vertex line[] = {
                sf::Vertex(project(cubeVertices[a]), sf::Color::White),
                sf::Vertex(project(cubeVertices[b]), sf::Color::White)
            };
            this->window.draw(line, 2, sf::Lines);
        };
        struct Edge {
            int a, b;
            bool toBeDrawn = false;
        };
        int edgeDefinitions[12][2] = {
            { 0, 1},
            { 1, 2},
            { 2, 3},
            { 3, 0},
            { 4, 5},
            { 5, 6},
            { 6, 7},
            { 7, 4},
            { 0, 4},
            { 1, 5},
            { 2, 6},
            { 3, 7}
        };
        std::vector<Edge> edges;
        // find the edges that make the 3 faces closest to the camera (highest z)
        // and draw them. This is a simple way to hide the back faces. 
        for (int i = 0; i < 12; i++) {
            edges.push_back({ edgeDefinitions[i][0], edgeDefinitions[i][1], false});
        }
        struct Face{
            Edge* edges[4];
        };
        std::vector<Face> faces;
        faces.push_back({&edges[0], &edges[1], &edges[2], &edges[3]});
        faces.push_back({&edges[4], &edges[5], &edges[6], &edges[7]});
        faces.push_back({&edges[8], &edges[9], &edges[10], &edges[11]});
        faces.push_back({&edges[0], &edges[4], &edges[8], &edges[3]});
        faces.push_back({&edges[1], &edges[5], &edges[9], &edges[2]});
        faces.push_back({&edges[6], &edges[10], &edges[11], &edges[7]});
        std::sort(faces.begin(), faces.end(), [&cubeVertices](Face a, Face b) -> bool {
            float zA = 0;
            float zB = 0;
            for (int i = 0; i < 4; i++) {
                zA += cubeVertices[a.edges[i]->a].z;
                zB += cubeVertices[b.edges[i]->a].z;
            }
            return zA > zB;
        });
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                faces[i].edges[j]->toBeDrawn = true;
            }
        }
        for (int i = 0; i < 12; i++) {
            if(edges[i].toBeDrawn)
                drawEdge(edges[i].a, edges[i].b);
        }
        

        // sf::VertexArray triangle(sf::Triangles, 3);

        // // define the position of the triangle's points
        // triangle[0].position = sf::Vector2f(100.f, 100.f);
        // triangle[1].position = sf::Vector2f(200.f, 100.f);
        // triangle[2].position = sf::Vector2f(200.f, 200.f);

        // // define the color of the triangle's points
        // triangle[0].color = sf::Color::Red;
        // triangle[1].color = sf::Color::Blue;
        // triangle[2].color = sf::Color::Green;
        // this->window.draw(triangle);

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