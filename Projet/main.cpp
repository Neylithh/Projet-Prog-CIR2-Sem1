#include <SFML/Graphics.hpp>
#include <SFML/System/Angle.hpp> // Nécessaire pour sf::degrees
#include <mutex>
#include <iostream>
#include <optional>
#include <cmath>
#include <iomanip>
#include <sstream>
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

// --- PARAMÈTRES ---
unsigned int WINDOW_W = 1200;
unsigned int WINDOW_H = 900;
float ECHELLE = 0.011f;
float OFFSET_X = WINDOW_W / 2.0f + 150.0f;
float OFFSET_Y = WINDOW_H / 2.0f - 50.0f;

// Conversion Monde -> Écran
sf::Vector2f worldToScreen(float x, float y) {
    return sf::Vector2f(
        OFFSET_X + x * ECHELLE,
        OFFSET_Y - y * ECHELLE
    );
}

// --- GENERATEUR ---
void routine_generateur(std::vector<Aeroport*>& aeres, CCR& ccr) {
    int id = 100;
    while (true) {
        simuler_pause(3000);

        Aeroport* dep = aeres[rand() % aeres.size()];
        Aeroport* arr = aeres[rand() % aeres.size()];
        if (dep == arr) continue;

        std::string nom = "VOL-" + std::to_string(id++);
        Avion* a = new Avion(nom, 300.f, 10000.f, 5.f, dep->position);

        {
            std::lock_guard<std::mutex> lock(mtxFlotte);
            flotte.push_back(a);
        }

        std::thread t(routine_avion, std::ref(*a), std::ref(*dep), std::ref(*arr), std::ref(ccr));
        t.detach();

        std::cout << "--- [GENERATEUR] " << nom << " : " << dep->nom << " -> " << arr->nom << " ---\n";
    }
}

int main() {
    std::srand(static_cast<unsigned int>(time(0)));

    CCR ccr;
    std::thread tCCR(routine_ccr, std::ref(ccr));

    Aeroport cdg("CDG", Position(0, 0, 0));
    Aeroport lhr("LHR", Position(-20000, 30000, 0));
    Aeroport mad("MAD", Position(-10000, -40000, 0));
    std::vector<Aeroport*> aeres = { &cdg, &lhr, &mad };

    std::vector<std::thread> threadsInfra;
    for (auto* apt : aeres) {
        threadsInfra.emplace_back(routine_twr, std::ref(*(apt->twr)));
        threadsInfra.emplace_back(routine_app, std::ref(*(apt->app)));
    }

    std::thread tGen(routine_generateur, std::ref(aeres), std::ref(ccr));

    // FENETRE
    sf::RenderWindow window(sf::VideoMode({WINDOW_W, WINDOW_H}), "Radar SFML 3.0", sf::Style::Default);
    window.setFramerateLimit(60);

    // Chargement Fond
    sf::Texture textureFond;
    std::optional<sf::Sprite> spriteFond;
    if (textureFond.loadFromFile("img/carte.jpg")) {
        spriteFond.emplace(textureFond);
        spriteFond->setScale({ (float)WINDOW_W / textureFond.getSize().x, (float)WINDOW_H / textureFond.getSize().y });
    }

    // Chargement Avion
    if (!textureAvionGlobal.loadFromFile("img/avion.png")) {
        std::cerr << "ERREUR : img/avion.png introuvable.\n";
        
        // --- CORRECTION DU WARNING ICI ---
        // On vérifie le retour de la fonction resize pour satisfaire [[nodiscard]]
        if (!textureAvionGlobal.resize({32, 32})) {
            std::cerr << "Erreur critique: Impossible de créer la texture par défaut.\n";
        }
    }
    textureAvionGlobal.setSmooth(true);

    sf::Font font;
    if (!font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
         // Rien, tant pis pour le texte
    }

    // BOUCLE
    while (window.isOpen()) {
        
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (wheel->wheel == sf::Mouse::Wheel::Vertical) {
                    if (wheel->delta > 0) ECHELLE *= 1.1f; else ECHELLE *= 0.9f;
                }
            }
            else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                float newW = static_cast<float>(resized->size.x);
                float newH = static_cast<float>(resized->size.y);
                window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {newW, newH})));
                if (spriteFond.has_value()) {
                    spriteFond->setScale({ newW / spriteFond->getTexture().getSize().x, newH / spriteFond->getTexture().getSize().y });
                }
                OFFSET_X = newW / 2.0f + 150.0f;
                OFFSET_Y = newH / 2.0f - 50.0f;
            }
            else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    sf::Vector2f clickPos((float)mouseBtn->position.x, (float)mouseBtn->position.y);
                    avionSelectionne = nullptr;

                    std::lock_guard<std::mutex> lock(mtxFlotte);
                    for (Avion* a : flotte) {
                        if (a->getEtat() == EtatAvion::TERMINE) continue;
                        Position pos = a->getPosition();
                        sf::Vector2f posEcran = worldToScreen(pos.getX(), pos.getY());
                        
                        float dx = clickPos.x - posEcran.x;
                        float dy = clickPos.y - posEcran.y;
                        
                        if (dx*dx + dy*dy < 225.f) { 
                            avionSelectionne = a;
                            std::cout << "Selection : " << a->getNom() << std::endl;
                            break;
                        }
                    }
                }
            }
        }

        window.clear(sf::Color(20, 20, 30));
        if (spriteFond.has_value()) window.draw(*spriteFond);

        // AEROPORTS
        for (auto* apt : aeres) {
            sf::Vector2f posEcran = worldToScreen(apt->position.getX(), apt->position.getY());
            sf::CircleShape point(5.f);
            point.setFillColor(sf::Color::Green); point.setOutlineColor(sf::Color::Black); point.setOutlineThickness(1.f);
            point.setOrigin({5.f, 5.f}); point.setPosition(posEcran); window.draw(point);

            sf::Text txt(font, apt->nom, 14);
            txt.setStyle(sf::Text::Bold); txt.setOutlineColor(sf::Color::Black); txt.setOutlineThickness(1.f);
            txt.setPosition({posEcran.x + 10.f, posEcran.y - 10.f}); window.draw(txt);
        }

        // AVIONS
        {
            std::lock_guard<std::mutex> lock(mtxFlotte);
            auto it = flotte.begin();
            while (it != flotte.end()) {
                Avion* a = *it;
                if (a->getEtat() == EtatAvion::TERMINE) { it = flotte.erase(it); continue; }

                Position pos = a->getPosition();
                sf::Vector2f posEcran = worldToScreen(pos.getX(), pos.getY());

                sf::Sprite spriteAvion(textureAvionGlobal);
                sf::Vector2u tailleTex = textureAvionGlobal.getSize();
                spriteAvion.setOrigin({tailleTex.x / 2.f, tailleTex.y / 2.f});
                spriteAvion.setPosition(posEcran);
                
                // Scale 0.1f (Petit avion)
                spriteAvion.setScale({0.1f, 0.1f}); 

                std::vector<Position> traj = a->getTrajectoire();
                if (!traj.empty()) {
                    Position cible = traj.front();
                    float dx = cible.getX() - pos.getX();
                    float dy = -(cible.getY() - pos.getY()); 

                    float angleRad = std::atan2(dy, dx);
                    float angleDeg = angleRad * 180.f / static_cast<float>(M_PI);
                    spriteAvion.setRotation(sf::degrees(angleDeg + 90.f));
                }

                EtatAvion etat = a->getEtat();
                if (a == avionSelectionne) spriteAvion.setColor(sf::Color::Magenta);
                else if (etat == EtatAvion::EN_ROUTE) spriteAvion.setColor(sf::Color::Cyan);
                else if (etat == EtatAvion::EN_APPROCHE) spriteAvion.setColor(sf::Color::Yellow);
                else if (etat == EtatAvion::ATTERRISSAGE) spriteAvion.setColor(sf::Color::Red);
                else spriteAvion.setColor(sf::Color::White);

                window.draw(spriteAvion);

                std::string label = a->getNom();
                sf::Text txt(font, label, 10);
                txt.setOutlineColor(sf::Color::Black); txt.setOutlineThickness(1.f);
                txt.setPosition({posEcran.x + 15.f, posEcran.y - 15.f});
                window.draw(txt);

                ++it;
            }
        }

        // UI
        if (avionSelectionne != nullptr) {
            bool existe = false;
            Position p;
            std::string n;
            EtatAvion e;
            {
                std::lock_guard<std::mutex> lock(mtxFlotte);
                auto it = std::find(flotte.begin(), flotte.end(), avionSelectionne);
                if (it != flotte.end()) {
                    existe = true; p = avionSelectionne->getPosition(); n = avionSelectionne->getNom(); e = avionSelectionne->getEtat();
                } else { avionSelectionne = nullptr; }
            }

            if (existe) {
                sf::RectangleShape panel({220.f, 130.f});
                panel.setFillColor(sf::Color(0, 0, 0, 200));
                panel.setOutlineColor(sf::Color::Magenta);
                panel.setOutlineThickness(2.f);
                panel.setPosition({10.f, 10.f});

                std::stringstream ss;
                ss << "AVION: " << n << "\n" << "----------------\n"
                   << std::fixed << std::setprecision(0)
                   << "X: " << p.getX() << " | Y: " << p.getY() << "\n"
                   << "ALT: " << p.getAltitude() << " m\n"
                   << "ETAT: " << (int)e;

                sf::Text txtInfo(font, ss.str(), 14);
                txtInfo.setFillColor(sf::Color::White);
                txtInfo.setPosition({20.f, 20.f});

                window.draw(panel); window.draw(txtInfo);
            }
        }

        window.display();
    }
    return 0;
}