#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>

// Programme corrige pour SFML 3.0 (Version stricte)
int main()
{
    // Correction MAJEURE pour SFML 3.0 :
    // sf::VideoMode demande maintenant explicitement un sf::Vector2u (unsigned int).
    // On ne peut plus passer simplement (800, 600).
    sf::Vector2u windowSize(800, 600);
    sf::VideoMode mode(windowSize);

    sf::RenderWindow window(mode, "SFML 3.0 Test");

    std::cout << "SFML 3.0 a bien demarre." << std::endl;

    sf::CircleShape circle(50.f);
    circle.setFillColor(sf::Color::Red);
    // Position avec Vector2f explicite pour eviter toute ambiguite
    circle.setPosition(sf::Vector2f(375.f, 275.f));

    while (window.isOpen())
    {
        // Gestion des evenements SFML 3.0
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (event->is<sf::Event::KeyPressed>())
            {
                // Recuperation securisee de l'evenement clavier
                const auto* keyEvent = event->getIf<sf::Event::KeyPressed>();
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Escape)
                {
                    window.close();
                }
            }
        }

        window.clear(sf::Color::Black);
        window.draw(circle);
        window.display();
    }

    std::cout << "Fin du programme." << std::endl;

    return 0;
}