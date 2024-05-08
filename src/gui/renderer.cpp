// Path: src/gui/renderer.h
#include "renderer.h"

Vertex crossProduct(const Vertex& v1, const Vertex& v2) {
    Vertex result;
    result.x = v1.y * v2.z - v1.z * v2.y;
    result.y = v1.z * v2.x - v1.x * v2.z;
    result.z = v1.x * v2.y - v1.y * v2.x;
    return result;
}

float dotProduct(const Vertex& v1, const Vertex& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vertex normalize(const Vertex& v) {
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    Vertex result;
    result.x = v.x / length;
    result.y = v.y / length;
    result.z = v.z / length;
    return result;
}

Vertex subtract(const Vertex& v1, const Vertex& v2) {
    Vertex result;
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    return result;
}

Vertex add(const Vertex& v1, const Vertex& v2) {
    Vertex result;
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    return result;
}

Vertex multiply(const Vertex& v, float scalar) {
    Vertex result;
    result.x = v.x * scalar;
    result.y = v.y * scalar;
    result.z = v.z * scalar;
    return result;
}

Vertex divide(const Vertex& v, float scalar) {
    Vertex result;
    result.x = v.x / scalar;
    result.y = v.y / scalar;
    result.z = v.z / scalar;
    return result;
}

Vertex getNormal(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    Vertex vector1 = subtract(v2, v1);
    Vertex vector2 = subtract(v3, v1);
    return normalize(crossProduct(vector1, vector2));
}

bool isBackFace(const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& camera) {
    Vertex normal = getNormal(v1, v2, v3);
    Vertex view = subtract(camera, v1);
    return dotProduct(normal, view) > 0;
}

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
    // float viewer_distance = 5;
    const int width = 720;
    const int height = 720;

    // Simple perspective projection
    float factor = fov / (this->viewer_distance + v.z);
    float x = v.x * factor + width / 2;
    float y = v.y * factor + height / 2;
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
    auto characterManager = new CharacterManager();
    Character* character = characterManager->getCharacterByName("Cube");
    characterManager->setCharacter(character);
    std::vector<Vertex> cubeVertices = {
        { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 }, // Front face
        { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 } // Back face
    };

    this->window.create(sf::VideoMode(720, 720), "TheCube", sf::Style::None);
    window.setVerticalSyncEnabled(true);
    glEnable(GL_TEXTURE_2D);
    bool visibleMouse = false;
    this->window.setFramerateLimit(60);
    this->window.setMouseCursorVisible(visibleMouse);
    window.setActive(true);

    int textSize = 24;
    if (!this->font.loadFromFile("/home/cube/.local/share/fonts/Hack-Regular.ttf")) {
        this->logger->error("Could not load font");
    }
    this->logger->log("Renderer initialized. Starting Window...", true);
    bool running = true;
    // while (this->window.isOpen()) {
    Vertex camera = { 0, 0, -this->viewer_distance };
    while (running) {
        for (auto event = sf::Event {}; this->window.pollEvent(event);) {
            if (event.type == sf::Event::Closed) {
                // this->window.close();
                running = false;
            }
        }
        // this->window.clear(sf::Color::Black);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cubeVertices = this->rotateY(0.001f, cubeVertices);
        cubeVertices = this->rotateX(0.007f, cubeVertices);
        // cubeVertices = this->rotateZ(0.017f, cubeVertices);
        this->viewer_distance += this->viewer_distance_increment ? 0.03 : -0.03;
        if (this->viewer_distance > 25 || this->viewer_distance < 5)
            this->viewer_distance_increment = !this->viewer_distance_increment;
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
        faces.push_back({&edges[0], &edges[1], &edges[3], &edges[2]});
        faces.push_back({&edges[6], &edges[7], &edges[4], &edges[5]});
        faces.push_back({&edges[8], &edges[0], &edges[9], &edges[4]});
        faces.push_back({&edges[5], &edges[10], &edges[1], &edges[9]});
        faces.push_back({&edges[10], &edges[2], &edges[11], &edges[6]});
        faces.push_back({&edges[3], &edges[8], &edges[7], &edges[11]});
        for (int i = 0; i < faces.size(); i++) {
            if(!isBackFace(cubeVertices[faces[i].edges[0]->a], cubeVertices[faces[i].edges[1]->a], cubeVertices[faces[i].edges[2]->a], camera)){
                faces[i].edges[0]->toBeDrawn = true;
                faces[i].edges[1]->toBeDrawn = true;
                faces[i].edges[2]->toBeDrawn = true;
                faces[i].edges[3]->toBeDrawn = true;
            }
        }
        for (int i = 0; i < 12; i++) {
            if(edges[i].toBeDrawn)
                drawEdge(edges[i].a, edges[i].b);
        }

        // animate normal vectors for each face
        for (int i = 0; i < faces.size(); i++) {
            Vertex normal = getNormal(cubeVertices[faces[i].edges[0]->a], cubeVertices[faces[i].edges[1]->a], cubeVertices[faces[i].edges[2]->a]);
            Vertex center = divide(add(add(add(cubeVertices[faces[i].edges[0]->a], cubeVertices[faces[i].edges[1]->a]), cubeVertices[faces[i].edges[2]->a]), cubeVertices[faces[i].edges[3]->a]), 4);
            Vertex normalEnd = add(center, multiply(normal, 0.5));
            sf::Vertex line[] = {
                sf::Vertex(project(center), sf::Color::Red),
                sf::Vertex(project(normalEnd), sf::Color::Red)
            };
            this->window.draw(line, 2, sf::Lines);
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