#include <SFML/Graphics.hpp>
#include <SFML/System/Angle.hpp>
#include <mutex>
#include <iostream>
#include <optional>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include "avion.hpp"
#include "thread.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- GLOBALES ---
std::vector<Avion*> flotte;
std::mutex mtxFlotte;
Avion* avionSelectionne = nullptr;
sf::Texture textureAvionGlobal;

// --- PARAMETRES DE CALIBRAGE ---
// J'ai pré-réglé ces valeurs pour être plus proche de votre capture d'écran
unsigned int WINDOW_W = 1100;
unsigned int WINDOW_H = 1000;
float ECHELLE = 0.00068f; // Réduit (était 0.00085) pour rapprocher les points
float OFFSET_X = 600.0f;
float OFFSET_Y = 280.0f;

// Conversion Monde -> Écran
sf::Vector2f worldToScreen(float x, float y) {
    return sf::Vector2f(OFFSET_X + x * ECHELLE, OFFSET_Y - y * ECHELLE);
}

// --- GENERATEUR ---
void routine_generateur(std::vector<Aeroport*>& aeres, CCR& ccr) {
    int id = 100;
    while (true) {
        simuler_pause(3000);
        Aeroport* dep = aeres[rand() % aeres.size()];
        Aeroport* arr = aeres[rand() % aeres.size()];

        if (dep == arr) continue;
        if (dep->position.distance(arr->position) < 40000) continue;

        std::string nom = "AFR-" + std::to_string(id++);
        Avion* a = new Avion(nom, 280.f, 15000.f, 5.f, dep->position);

        {
            std::lock_guard<std::mutex> lock(mtxFlotte);
            flotte.push_back(a);
        }

        std::thread t(routine_avion, std::ref(*a), std::ref(*dep), std::ref(*arr), std::ref(ccr));
        t.detach();
        std::cout << "--- [GEN] " << nom << " : " << dep->nom << " -> " << arr->nom << " ---\n";
    }
}

int main() {
    std::srand(static_cast<unsigned int>(time(0)));
    std::cout << "DOSSIER D'EXECUTION : " << std::filesystem::current_path() << std::endl;

    std::cout << "\n=== MODE CALIBRAGE ACTIVE ===" << std::endl;
    std::cout << "Utilisez les FLECHES pour deplacer les avions (Nord/Sud/Est/Ouest)" << std::endl;
    std::cout << "Utilisez PageUP / PageDOWN pour modifier l'Echelle (Zoom)" << std::endl;
    std::cout << "=============================\n" << std::endl;

    CCR ccr;
    std::thread tCCR(routine_ccr, std::ref(ccr));

    // --- AEROPORTS ---
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
    Aeroport bes("BES", Position(-855000, -65000, 0)); // Brest

    std::vector<Aeroport*> aeres = { &cdg, &ory, &lil, &sxb, &lys, &nce, &mrs, &tls, &bod, &nte, &bes };

    std::vector<std::thread> threadsInfra;
    for (auto* apt : aeres) {
        threadsInfra.emplace_back(routine_twr, std::ref(*(apt->twr)));
        threadsInfra.emplace_back(routine_app, std::ref(*(apt->app)));
    }

    std::thread tGen(routine_generateur, std::ref(aeres), std::ref(ccr));

    sf::RenderWindow window(sf::VideoMode({ WINDOW_W, WINDOW_H }), "Controle Aerien France", sf::Style::Default);
    window.setFramerateLimit(60);

    // --- CHARGEMENT IMAGE ---
    sf::Texture textureFond;
    std::optional<sf::Sprite> spriteFond;
    bool imageChargee = false;

    if (textureFond.loadFromFile("img/france.png")) {
        imageChargee = true;
        spriteFond.emplace(textureFond);
        float ratioX = (float)WINDOW_W / textureFond.getSize().x;
        float ratioY = (float)WINDOW_H / textureFond.getSize().y;
        float scaleUtilise = std::min(ratioX, ratioY);
        spriteFond->setScale({ scaleUtilise, scaleUtilise });
        spriteFond->setPosition({
            (WINDOW_W - textureFond.getSize().x * scaleUtilise) / 2.f,
            (WINDOW_H - textureFond.getSize().y * scaleUtilise) / 2.f
            });

        // --- PRE-CALCUL APPROXIMATIF ---
        // J'ai ajusté ces valeurs : 0.50 (Centre X) et 0.23 (Plus haut en Y)
        OFFSET_X = spriteFond->getPosition().x + (textureFond.getSize().x * scaleUtilise * 0.50f);
        OFFSET_Y = spriteFond->getPosition().y + (textureFond.getSize().y * scaleUtilise * 0.23f);
        ECHELLE = 0.00068f * scaleUtilise; // Echelle réduite pour resserrer les points
    }
    else {
        std::cerr << "ERREUR: 'img/france.png' introuvable." << std::endl;
    }

    if (!textureAvionGlobal.loadFromFile("img/avion.png")) {
        sf::Image img; img.resize({ 32, 32 }, sf::Color::White);
        textureAvionGlobal.loadFromImage(img);
    }
    textureAvionGlobal.setSmooth(true);

    sf::Font font;
    bool fontLoaded = false;
    std::vector<std::string> fontsToTry = { "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/seguiemj.ttf", "C:/Windows/Fonts/tahoma.ttf" };
    for (const auto& path : fontsToTry) { if (font.openFromFile(path)) { fontLoaded = true; break; } }

    // --- BOUCLE ---
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            // Clic Souris (Sélection)
            else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    sf::Vector2f clickPos((float)mouseBtn->position.x, (float)mouseBtn->position.y);
                    avionSelectionne = nullptr;
                    std::lock_guard<std::mutex> lock(mtxFlotte);
                    for (Avion* a : flotte) {
                        if (a->getEtat() == EtatAvion::TERMINE) continue;
                        sf::Vector2f posEcran = worldToScreen(a->getPosition().getX(), a->getPosition().getY());
                        float dx = clickPos.x - posEcran.x;
                        float dy = clickPos.y - posEcran.y;
                        if (dx * dx + dy * dy < 400.f) { avionSelectionne = a; break; }
                    }
                }
            }
        }

        // --- CALIBRAGE CLAVIER EN TEMPS REEL ---
        bool changed = false;
        float moveSpeed = 2.0f; // Vitesse de déplacement

        // Deplacer la carte (Offset)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) { OFFSET_X -= moveSpeed; changed = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) { OFFSET_X += moveSpeed; changed = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) { OFFSET_Y -= moveSpeed; changed = true; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) { OFFSET_Y += moveSpeed; changed = true; }

        // Modifier l'échelle (Zoom/Dezoom) - PageUp / PageDown
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageUp)) { ECHELLE *= 1.005f; changed = true; } // Ecarte les points
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::PageDown)) { ECHELLE *= 0.995f; changed = true; } // Resserre les points

        if (changed) {
            // Optionnel : Afficher les valeurs dans la console pour les noter
            // std::cout << "OFFSET_X: " << OFFSET_X << " | OFFSET_Y: " << OFFSET_Y << " | ECHELLE: " << ECHELLE << std::endl;
        }

        window.clear(imageChargee ? sf::Color::White : sf::Color(100, 100, 100));
        if (spriteFond.has_value()) window.draw(*spriteFond);

        // Dessin Aéroports
        for (auto* apt : aeres) {
            sf::Vector2f posEcran = worldToScreen(apt->position.getX(), apt->position.getY());

            sf::CircleShape point(5.f);
            point.setFillColor(sf::Color::Blue);
            point.setOrigin({ 5.f, 5.f });
            point.setPosition(posEcran);
            window.draw(point);

            if (fontLoaded) {
                sf::Text txt(font, apt->nom, 12);
                txt.setFillColor(sf::Color::Black);
                txt.setStyle(sf::Text::Bold);
                txt.setPosition({ posEcran.x + 8.f, posEcran.y - 8.f });
                window.draw(txt);
            }
        }

        // Dessin Avions
        {
            std::lock_guard<std::mutex> lock(mtxFlotte);
            auto it = flotte.begin();
            while (it != flotte.end()) {
                Avion* a = *it;
                if (a->getEtat() == EtatAvion::TERMINE) { delete a; it = flotte.erase(it); continue; }

                Position pos = a->getPosition();
                sf::Vector2f posEcran = worldToScreen(pos.getX(), pos.getY());

                sf::Sprite spriteAvion(textureAvionGlobal);
                sf::Vector2u tailleTex = textureAvionGlobal.getSize();
                spriteAvion.setOrigin({ tailleTex.x / 2.f, tailleTex.y / 2.f });
                spriteAvion.setPosition(posEcran);
                spriteAvion.setScale({ 0.08f, 0.08f });

                std::vector<Position> traj = a->getTrajectoire();
                if (!traj.empty()) {
                    float dx = traj.front().getX() - pos.getX();
                    float dy = -(traj.front().getY() - pos.getY());
                    spriteAvion.setRotation(sf::degrees(std::atan2(dy, dx) * 180.f / (float)M_PI + 90.f));
                }

                if (a == avionSelectionne) spriteAvion.setColor(sf::Color::Magenta);
                else spriteAvion.setColor(sf::Color::Black);

                window.draw(spriteAvion);
                ++it;
            }
        }

        if (avionSelectionne && fontLoaded) {
            sf::RectangleShape panel({ 200.f, 100.f });
            panel.setFillColor(sf::Color(0, 0, 0, 150));
            panel.setPosition({ 10.f, 10.f });
            window.draw(panel);

            sf::Text txt(font, "Avion: " + avionSelectionne->getNom(), 14);
            txt.setFillColor(sf::Color::White);
            txt.setPosition({ 20.f, 20.f });
            window.draw(txt);
        }

        window.display();
    }
    return 0;
}