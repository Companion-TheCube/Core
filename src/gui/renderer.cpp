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


int Renderer::thread()
{
    auto characterManager = new CharacterManager();
    Character* character = characterManager->getCharacterByName("Cube");
    characterManager->setCharacter(character);

    this->window.create(sf::VideoMode(720, 720), "TheCube", sf::Style::None);
    window.setVerticalSyncEnabled(true);
    this->window.setFramerateLimit(60);
    glewExperimental = GL_TRUE; // Enable full GLEW functionality
    if (GLEW_OK != glewInit()) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }
    printf("%s\n", glGetString(GL_VERSION));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glClearDepth(1.f);
    glMatrixMode(GL_PROJECTION);
    gluPerspective(90.0f, float(window.getSize().x) / float(window.getSize().y), 1.0f, 500.0f);

    bool visibleMouse = false;
    this->window.setMouseCursorVisible(visibleMouse);

    Shader edges("./shaders/edges.vs", "./shaders/edges.fs");
    Shader cube("./shaders/cube.vs", "./shaders/cube.fs");

    int textSize = 24;
    if (!this->font.loadFromFile("/home/cube/.local/share/fonts/Hack-Regular.ttf")) {
        this->logger->error("Could not load font");
    }

    Vertex vertices[] = {
        // Front face
        { -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
        { -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f },

        // Back face
        { -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.0f }
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0, // Front face
        4, 5, 6, 6, 7, 4, // Back face
        4, 0, 3, 3, 7, 4, // Left face
        1, 5, 6, 6, 2, 1, // Right face
        0, 1, 5, 5, 4, 0, // Bottom face
        3, 2, 6, 6, 7, 3 // Top face
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT, GL_LINE);

    this->logger->log("Renderer initialized. Starting Loop...", true);
    float rotation = 0;
    bool running = true;
    while (running) {
        for (auto event = sf::Event {}; this->window.pollEvent(event);) {
            if (event.type == sf::Event::Closed) {
                // this->window.close();
                running = false;
            }
        }
        this->window.setActive();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set clear color to black
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        edges.use();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 720.f / 720.f, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 6.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        edges.setMat4("projection", projection);
        edges.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);  // Start with the identity matrix
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // No translation
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate 45 degrees around the x-axis
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); // No scaling
        edges.setMat4("model", model);
        glDepthMask(GL_FALSE);
        glDrawElements(GL_LINE_LOOP, 36, GL_UNSIGNED_INT, 0);
        glDepthMask(GL_TRUE);
        cube.use();
        glm::mat4 projection2 = glm::perspective(glm::radians(45.0f), 720.f / 720.f, 0.1f, 100.0f);
        glm::mat4 view2 = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 6.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        cube.setMat4("projection2", projection2);
        cube.setMat4("view2", view2);
        glm::mat4 model2 = glm::mat4(1.0f);  // Start with the identity matrix
        model2 = glm::translate(model2, glm::vec3(0.0f, 0.0f, 0.0f)); // No translation
        model2 = glm::rotate(model2, glm::radians(rotation), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate 45 degrees around the x-axis
        model2 = glm::scale(model2, glm::vec3(1.0f, 1.0f, 1.0f)); // No scaling
        cube.setMat4("model2", model2);
        glDepthMask(GL_FALSE);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glDepthMask(GL_TRUE);
        glBindVertexArray(0);
        this->window.display();
        rotation+=1;
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    return 0;
}