#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <cmath>
#include <mutex>
#include <optional> 

// SFML 3.0 Includes
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include "avion.hpp"
#include "thread.hpp"

// ================= CONSTANTES VISUELLES =================
const int WINDOW_WIDTH = 10000;
const int WINDOW_HEIGHT = 10000 ;
const float PI = 3.14159265f;

// Facteurs d'échelle (Vos coordonnées vont de 0 à ~300000 en X)
const double SCALE_X = (double)WINDOW_WIDTH / 500000.0;
const double SCALE_Y = (double)WINDOW_HEIGHT / 400000.0;

// ================= OUTILS GRAPHIQUES =================
sf::Vector2f worldToScreen(const Position& pos) {
    float x = static_cast<float>(pos.getX() * SCALE_X) + 50.f;
    float y = static_cast<float>(pos.getY() * SCALE_Y) + 50.f;
    return sf::Vector2f(x, y);
}

// ================= GLOBALES =================
std::vector<std::thread> threads_infra;
std::vector<Avion*> flotte;
std::mutex mutexFlotte; 

// ================= MAIN =================
int main() {
    std::srand(static_cast<unsigned int>(time(NULL)));

    std::cout << "[INFO] Démarrage du simulateur SFML 3.0..." << std::endl;

    // --- 1. CONFIGURATION SFML ---
    sf::RenderWindow window(sf::VideoMode({(unsigned int)WINDOW_WIDTH, (unsigned int)WINDOW_HEIGHT}), "Simulateur ATC");
    window.setFramerateLimit(60);

    // Chargement des textures (chemins relatifs)
    sf::Texture textureAvion, textureFond;
    bool hasTextureAvion = textureAvion.loadFromFile("img/avion.png");
    bool hasTextureMap = textureFond.loadFromFile("img/carte.jpg"); 

    // Sprite de fond
    sf::Sprite spriteFond(textureFond);
    if (hasTextureMap) {
        sf::Vector2u sizeFond = textureFond.getSize();
        if (sizeFond.x > 0) {
            spriteFond.setScale({
                (float)WINDOW_WIDTH / sizeFond.x,
                (float)WINDOW_HEIGHT / sizeFond.y
            });
        }
    }

    // --- 2. CREATION INFRASTRUCTURE ---
    CCR ccr;
    // Les aéroports sont créés sur la pile du main, ils vivent jusqu'à la fin du programme
    Aeroport cdg("CDG", Position(110000, 30000, 0));
    Aeroport ory("ORY", Position(80000, -35000, 0));
    Aeroport lil("LIL", Position(182000, 345000, 0));
    Aeroport sxb("SXB", Position(830000, -60000, 0));
    Aeroport lys("LYS", Position(480000, -650000, 0));
    Aeroport nce("NCE", Position(838000, -1070000, 0));
    Aeroport mrs("MRS", Position(515000, -1160000, 0));
    Aeroport tls("TLS", Position(-62000, -1116000, 0));
    Aeroport bod("BOD", Position(-356000, -860000, 0));
    Aeroport nte("NTE", Position(-500000, -350000, 0));
    Aeroport bes("BES", Position(-855000, -65000, 0)); 

    // --- 3. LANCEMENT DES THREADS INFRA ---
    // Attention : on passe par référence car ccr/lille vivent dans le main
    threads_infra.emplace_back(routine_ccr, std::ref(ccr));
    threads_infra.emplace_back(routine_twr, std::ref(*lil.twr));
    threads_infra.emplace_back(routine_app, std::ref(*lil.app));
    threads_infra.emplace_back(routine_twr, std::ref(*cdg.twr));
    threads_infra.emplace_back(routine_app, std::ref(*cdg.app));
    threads_infra.emplace_back(routine_twr, std::ref(*mrs.twr));
    threads_infra.emplace_back(routine_app, std::ref(*mrs.app));

    // --- 4. THREAD GENERATEUR DE TRAFIC ---
    std::thread trafficGenerator([&]() {
        // Liste locale pour le tirage aléatoire
        std::vector<Aeroport*> aeroports = { &lil, &cdg, &mrs };
        int idAvion = 1;

        for (int i = 0; i < 6; ++i) { 
            std::this_thread::sleep_for(std::chrono::seconds(2)); 

            std::string nom = "AIRFRANCE-" + std::to_string(idAvion++);
            
            int idxDepart = std::rand() % aeroports.size();
            int idxDest;
            do { idxDest = std::rand() % aeroports.size(); } while (idxDest == idxDepart);

            Aeroport* depart = aeroports[idxDepart];
            Aeroport* destination = aeroports[idxDest];

            // Position départ décalée
            Position posDepart = depart->position;
            posDepart.setPosition(posDepart.getX(), posDepart.getY() - 5000, 5000);

            // L'avion est alloué dynamiquement (heap), il survit au thread
            Avion* nouvelAvion = new Avion(nom, 500.f, 20.f, 5000.f, 2.f, 3000.f, posDepart);
            nouvelAvion->setDestination(destination);

            ccr.prendreEnCharge(nouvelAvion);

            // CORRECTION CRITIQUE ICI : "aeroports" est passé par COPIE (pas de std::ref)
            // car le vecteur local va être détruit à la fin de la lambda.
            std::thread tPilote(routine_avion, 
                std::ref(*nouvelAvion), 
                std::ref(*depart), 
                std::ref(*destination), 
                std::ref(ccr), 
                aeroports // <--- PASSER PAR VALEUR (COPIE)
            );
            tPilote.detach();

            // Ajout thread-safe à la flotte pour l'affichage
            {
                std::lock_guard<std::mutex> lock(mutexFlotte);
                flotte.push_back(nouvelAvion);
            }
        }
    });
    trafficGenerator.detach();

    // --- 5. BOUCLE D'AFFICHAGE ---
    while (window.isOpen()) {
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
        std::vector<Aeroport*> listAero = { &lil, &cdg, &mrs };
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

        // Dessin Avions (Section Critique)
        {
            std::lock_guard<std::mutex> lock(mutexFlotte); 
            
            for (auto avion : flotte) {
                // Sécurité anti-crash : vérifier pointeur null
                if (avion == nullptr) continue;
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
                    spriteAvion.setRotation(sf::degrees(angle + 90.f)); 
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
                    triangle.setRotation(sf::degrees(angle));
                    window.draw(triangle);
                }
            }
        }

        window.display();
    }

    // Nettoyage final
    // Note : Dans une vraie app, il faudrait arrêter les threads proprement avant de delete.
    // Ici on quitte le main, l'OS nettoiera la mémoire restante.
    // On évite le delete manuel ici car les threads peuvent encore essayer d'accéder aux avions
    // pendant la fermeture, ce qui causerait un autre crash de fermeture.
    
    return 0;
}