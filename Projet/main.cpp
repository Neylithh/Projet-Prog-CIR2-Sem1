#include <SFML/Graphics.hpp>
#include <iostream>

int main()
{
    // Correction 1: Utiliser le constructeur sf::VideoMode() avec des parenthèses
    sf::RenderWindow window(sf::VideoMode(800, 600), "Test SFML - Ca Compile !");

    std::cout << "SFML a bien démarré." << std::endl;

    while (window.isOpen())
    {
        // Correction 2: Déclaration simple (pas de constructeur par défaut requis)
        sf::Event event;

        // La fonction pollEvent prend bien 1 argument (la référence à 'event')
        while (window.pollEvent(event))
        {
            // Correction 3 & 4: Utiliser l'énumération complète pour le type et la touche
            // (sf::EventType::Closed et sf::Keyboard::Key::Escape)
            if (event.type == sf::EventType::Closed ||
                (event.type == sf::EventType::KeyPressed && event.key.code == sf::Keyboard::Key::Escape))
                window.close();
        }

        window.clear(sf::Color::Black);
        window.display();
    }

    std::cout << "Fenêtre fermée. Fin du programme." << std::endl;

    return 0;
}