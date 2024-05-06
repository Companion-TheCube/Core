#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <SFML/Graphics.hpp>

int main()
{
    auto window = sf::RenderWindow{ { 720, 720u }, "CMake SFML Project", sf::Style::None };
    window.setFramerateLimit(60);
    sf::Font font;
    if (!font.loadFromFile("/home/cube/.local/share/fonts/Hack-Regular.ttf"))
    {
        // error...
        std::cout<<"error"<<std::endl;
    }

    bool visibleMouse = false;

    while (window.isOpen())
    {
        for (auto event = sf::Event{}; window.pollEvent(event);)
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }
        window.setMouseCursorVisible(visibleMouse);
        visibleMouse = !visibleMouse;

        window.clear(sf::Color::Black);

        sf::VertexArray triangle(sf::Triangles, 3);


        // define the position of the triangle's points
        triangle[0].position = sf::Vector2f(100.f, 100.f);
        triangle[1].position = sf::Vector2f(200.f, 100.f);
        triangle[2].position = sf::Vector2f(200.f, 200.f);

        // define the color of the triangle's points
        triangle[0].color = sf::Color::Red;
        triangle[1].color = sf::Color::Blue;
        triangle[2].color = sf::Color::Green;
        window.draw(triangle);

        sf::Text text;

        // select the font
        text.setFont(font); // font is a sf::Font

        // set the string to display
        text.setString("Hello world");

        // set the character size
        text.setCharacterSize(24); // in pixels, not points!

        // set the color
        text.setFillColor(sf::Color::Red);

        // set the text style
        text.setStyle(sf::Text::Bold | sf::Text::Underlined);

        // inside the main loop, between window.clear() and window.display()
        window.draw(text);

        // window.clear();
        window.display();
    }
}