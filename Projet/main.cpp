#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <cmath>
#include <mutex>
#include <optional> // Nécessaire pour pollEvent en SFML 3

// SFML 3.0 Includes
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include "avion.hpp"
#include "thread.hpp"

// ================= CONSTANTES VISUELLES =================
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float PI = 3.14159265f;

// Facteurs d'échelle (Vos coordonnées vont de 0 à ~300000 en X)
const double SCALE_X = (double)WINDOW_WIDTH / 350000.0;
const double SCALE_Y = (double)WINDOW_HEIGHT / 150000.0;

// ================= OUTILS GRAPHIQUES =================

// Convertit une position simulation (mètres) en position écran (pixels)
sf::Vector2f worldToScreen(const Position& pos) {
    float x = static_cast<float>(pos.getX() * SCALE_X) + 50.f;
    float y = static_cast<float>(pos.getY() * SCALE_Y) + 50.f;
    return sf::Vector2f(x, y);
}

// ================= GLOBALES (Pour le Main seulement) =================
std::vector<std::thread> threads_infra;
std::vector<Avion*> flotte;
std::mutex mutexFlotte; 

// ================= MAIN =================
int main() {
    std::srand(static_cast<unsigned int>(time(NULL)));

    // --- 1. CONFIGURATION SFML ---
    sf::RenderWindow window(sf::VideoMode({(unsigned int)WINDOW_WIDTH, (unsigned int)WINDOW_HEIGHT}), "Simulateur ATC - SFML 3.0");
    window.setFramerateLimit(60);

    // Chargement des textures
    sf::Texture textureAvion, textureFond;
    
    // Chemins relatifs (vérifiez que le dossier img est bien à côté de l'exécutable)
    bool hasTextureAvion = textureAvion.loadFromFile("img/avion.png");
    bool hasTextureMap = textureFond.loadFromFile("img/carte.jpg"); 

    // Sprite de fond
    sf::Sprite spriteFond(textureFond);
    
    sf::Vector2u sizeFond = textureFond.getSize();
    if (sizeFond.x > 0) {
        // CORRECTION 1 : setScale prend désormais un Vector2f (utilisez les accolades {})
        spriteFond.setScale({
            (float)WINDOW_WIDTH / sizeFond.x,
            (float)WINDOW_HEIGHT / sizeFond.y
        });
    }

    // --- 2. CREATION INFRASTRUCTURE (Logique Métier) ---
    CCR ccr;
    Aeroport lille("Lille", Position(10000, 10000, 0), 20000.0f);
    Aeroport paris("Paris", Position(90000, 50000, 0), 20000.0f);
    Aeroport marseille("Marseille", Position(300000, 100000, 0), 20000.0f);

    // --- 3. LANCEMENT DES THREADS INFRA ---
    threads_infra.emplace_back(routine_ccr, std::ref(ccr));
    threads_infra.emplace_back(routine_twr, std::ref(*lille.twr));
    threads_infra.emplace_back(routine_app, std::ref(*lille.app));
    threads_infra.emplace_back(routine_twr, std::ref(*paris.twr));
    threads_infra.emplace_back(routine_app, std::ref(*paris.app));
    threads_infra.emplace_back(routine_twr, std::ref(*marseille.twr));
    threads_infra.emplace_back(routine_app, std::ref(*marseille.app));

    // --- 4. THREAD GENERATEUR DE TRAFIC ---
    std::thread trafficGenerator([&]() {
        std::vector<Aeroport*> aeroports = { &lille, &paris, &marseille };
        int idAvion = 1;

        for (int i = 0; i < 6; ++i) { 
            std::this_thread::sleep_for(std::chrono::seconds(2)); 

            std::string nom = "AIRFRANCE-" + std::to_string(idAvion++);
            
            int idxDepart = std::rand() % aeroports.size();
            int idxDest;
            do { idxDest = std::rand() % aeroports.size(); } while (idxDest == idxDepart);

            Aeroport* depart = aeroports[idxDepart];
            Aeroport* destination = aeroports[idxDest];

            Position posDepart = depart->position;
            posDepart.setPosition(posDepart.getX(), posDepart.getY() - 5000, 5000);

            Avion* nouvelAvion = new Avion(nom, 500.f, 20.f, 5000.f, 2.f, 3000.f, posDepart);
            nouvelAvion->setDestination(destination);

            ccr.prendreEnCharge(nouvelAvion);

            std::thread tPilote(routine_avion, std::ref(*nouvelAvion), std::ref(*depart), std::ref(*destination), std::ref(ccr), std::ref(aeroports));
            tPilote.detach();

            {
                std::lock_guard<std::mutex> lock(mutexFlotte);
                flotte.push_back(nouvelAvion);
            }
        }
    });
    trafficGenerator.detach();

    // --- 5. BOUCLE D'AFFICHAGE ---
    while (window.isOpen()) {
        // SFML 3.0 : pollEvent retourne un std::optional
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }

        window.clear(sf::Color(30, 30, 30)); 

        if (hasTextureMap) window.draw(spriteFond);

        // Dessin Aéroports
        std::vector<Aeroport*> listAero = { &lille, &paris, &marseille };
        for (auto aero : listAero) {
            sf::Vector2f p = worldToScreen(aero->position);
            
            sf::CircleShape point(5.f);
            point.setFillColor(sf::Color::Red);
            point.setOrigin({5.f, 5.f});
            point.setPosition(p);
            window.draw(point);

            sf::CircleShape zone(50.f); 
            zone.setFillColor(sf::Color(255, 0, 0, 30)); 
            zone.setOutlineColor(sf::Color::Red);
            zone.setOutlineThickness(1.f);
            zone.setOrigin({50.f, 50.f});
            zone.setPosition(p);
            window.draw(zone);
        }

        // Dessin Avions
        {
            std::lock_guard<std::mutex> lock(mutexFlotte); 
            
            for (auto avion : flotte) {
                if (avion->getEtat() == EtatAvion::TERMINE) continue;

                Position pos = avion->getPosition();
                sf::Vector2f screenPos = worldToScreen(pos);
                float angle = 0.f;

                std::vector<Position> traj = avion->getTrajectoire();
                if (!traj.empty()) {
                    double dx = traj.front().getX() - pos.getX();
                    double dy = traj.front().getY() - pos.getY();
                    angle = (float)(std::atan2(dy, dx) * 180.0 / PI);
                }

                if (hasTextureAvion) {
                    sf::Sprite spriteAvion(textureAvion);
                    
                    // CORRECTION 2 : Utiliser sf::degrees() pour la rotation
                    spriteAvion.setRotation(sf::degrees(angle + 90.f)); 
                    
                    // CORRECTION 3 : setScale prend un Vector2f ({x, y})
                    spriteAvion.setScale({0.05f, 0.05f}); 
                    
                    sf::Vector2u size = textureAvion.getSize();
                    spriteAvion.setOrigin({(float)size.x / 2.f, (float)size.y / 2.f});
                    
                    spriteAvion.setPosition(screenPos);
                    
                    if (avion->estEnUrgence()) {
                        spriteAvion.setColor(sf::Color::Red);
                    }

                    window.draw(spriteAvion);
                } else {
                    sf::ConvexShape triangle;
                    triangle.setPointCount(3);
                    triangle.setPoint(0, {10.f, 0.f});
                    triangle.setPoint(1, {-5.f, 5.f});
                    triangle.setPoint(2, {-5.f, -5.f});
                    
                    triangle.setFillColor(avion->estEnUrgence() ? sf::Color::Red : sf::Color::Cyan);
                    triangle.setPosition(screenPos);
                    
                    // CORRECTION 2 (bis)
                    triangle.setRotation(sf::degrees(angle));
                    
                    window.draw(triangle);
                }
            }
        }

        window.display();
    }

    // Nettoyage mémoire des avions
    for (auto avion : flotte) delete avion;

    return 0;
}