#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <cmath>
#include <mutex>
#include <optional> 
#include <iomanip>
#include <random>


// SFML 3.0 Includes
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include "avion.hpp"
#include "thread.hpp"

// ================= CONSTANTES VISUELLES =================

const float PI = 3.14159265f;
unsigned int WINDOW_WIDTH = 1100;
unsigned int WINDOW_HEIGHT = 1000;

// Echelle adaptée pour faire tenir la France dans la fenêtre
float ECHELLE = 0.00068f;
float OFFSET_X = 600.0f;
float OFFSET_Y = 280.0f;

// Conversion Monde (Mètres) -> Écran (Pixels)
sf::Vector2f worldToScreen(Position pos) {
    // SFML 3 utilise strictement des floats pour les vecteurs
    return sf::Vector2f(
        OFFSET_X + static_cast<float>(pos.getX()) * ECHELLE,
        OFFSET_Y - static_cast<float>(pos.getY()) * ECHELLE
    );
}

// Fonction utilitaire pour adapter un sprite à la fenêtre
void adapterFondFenetre(sf::Sprite& sprite, const sf::Texture& texture) {
    sf::Vector2u size = texture.getSize();
    if (size.x > 0 && size.y > 0) {
        sprite.setTexture(texture);
        sprite.setScale({ 1.f, 1.f }); // Reset scale avec accolades

        float scaleX = (float)WINDOW_WIDTH / (float)size.x;
        float scaleY = (float)WINDOW_HEIGHT / (float)size.y;

        // CORRECTION SFML 3 : Accolades obligatoires pour les vecteurs
        sprite.setScale({ scaleX, scaleY });
    }
}

// ================= GLOBALES =================
std::vector<std::thread> threads_infra;
std::vector<Avion*> flotte;
std::mutex mutexFlotte;

Avion* avionSelectionne = nullptr;
Aeroport* aeroportVue = nullptr;

// ================= MAIN =================
int main() {
    std::srand(static_cast<unsigned int>(time(NULL)));

    std::cout << "===============================================\n";
    std::cout << "   SIMULATEUR ATC - VISUALISATION SFML 3.0     \n";
    std::cout << "===============================================\n";

    // --- 2. CONFIGURATION SFML ---
    sf::RenderWindow window(sf::VideoMode({ (unsigned int)WINDOW_WIDTH, (unsigned int)WINDOW_HEIGHT }), "Simulateur ATC");
    window.setFramerateLimit(60);

    sf::View vueDefaut = window.getDefaultView();
    sf::View vueMonde = window.getDefaultView();
    float niveauZoomActuel = 1.0f;

    // --- CHARGEMENT RESSOURCES ---
    sf::Texture textureCarte;
    // Utilisation d'un chemin relatif simple. Assurez-vous que le dossier "img" est à côté de l'exe
    bool hasTextureMap = textureCarte.loadFromFile("img/carte.jpg");

    // CHARGEMENT POLICE (CRITIQUE POUR SFML 3)
    sf::Font font;
    bool hasFont = false;
    // Tente de charger Arial depuis Windows si le fichier local n'existe pas
    if (font.openFromFile("img/arial.ttf")) hasFont = true;
    if (!hasFont) std::cerr << "[ERREUR] Police introuvable. Le texte sera absent.\n";

    // CORRECTION SFML 3 : Initialisation du Sprite avec la texture immédiatement
    sf::Sprite spriteFond(textureCarte);
    if (hasTextureMap) {
        adapterFondFenetre(spriteFond, textureCarte);
    }

    // --- 3. CREATION INFRASTRUCTURE ---
    CCR ccr;
    Aeroport cdg("Paris", Position(0, 0, 0), 80000.0f);
    //Aeroport ory("ORY", Position(-5000, -35000, 0), 20000.0f);
    Aeroport lil("Lille", Position(93000, 331000, 0), 80000.0f);
    //Aeroport sxb("SXB", Position(550000, -30000, 0), 20000.0f);
    //Aeroport lys("LYS", Position(345000, -540000, 0), 20000.0f);
    //Aeroport nce("NCE", Position(675000, -900000, 0), 20000.0f);
    Aeroport mrs("Marseille", Position(432000, -785000, 0), 80000.0f);
    //Aeroport tls("TLS", Position(-60000, -810000, 0), 20000.0f);
    //Aeroport bod("BOD", Position(-300000, -675000, 0), 20000.0f);
    //Aeroport nte("NTE", Position(-390000, -345000, 0), 20000.0f);
    //Aeroport bes("BES", Position(-750000, -120000, 0), 20000.0f);

    //std::vector<Aeroport*> listeAeroports = { &cdg, &ory, &lil, &sxb, &lys, &nce, &mrs, &tls, &bod, &nte, &bes };
    std::vector<Aeroport*> listeAeroports = { &cdg, &lil, &mrs };

    // --- 4. THREADS INFRA ---
    threads_infra.emplace_back(routine_ccr, std::ref(ccr));
    for (Aeroport* aero : listeAeroports) {
        threads_infra.emplace_back(routine_twr, std::ref(*aero->twr));
        threads_infra.emplace_back(routine_app, std::ref(*aero->app));
    }

    // --- 5. GENERATEUR TRAFIC ---
    std::thread trafficGenerator([&]() {
        int idAvion = 1;
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::string nom = "AF-" + std::to_string(idAvion++);
            int idxDepart = std::rand() % listeAeroports.size();
            int idxDest;
            do { idxDest = std::rand() % listeAeroports.size(); } while (idxDest == idxDepart);

            Aeroport* depart = listeAeroports[idxDepart];
            Aeroport* destination = listeAeroports[idxDest];

            Position posDepart = depart->position;
            posDepart.setPosition(posDepart.getX(), posDepart.getY() - 5000, 10000);

            Avion* nouvelAvion = new Avion(nom, 4000.f, 5.f, 5000.f, 2.f, 5000.f, posDepart);
            nouvelAvion->setDestination(destination);

            ccr.prendreEnCharge(nouvelAvion);

            std::thread tPilote(routine_avion,
                std::ref(*nouvelAvion), std::ref(*depart), std::ref(*destination),
                std::ref(ccr), listeAeroports
            );
            tPilote.detach();

            {
                std::lock_guard<std::mutex> lock(mutexFlotte);
                flotte.push_back(nouvelAvion);
            }
        }
        });
    trafficGenerator.detach();

    // --- 6. BOUCLE D'AFFICHAGE ---
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
            else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    window.setView(vueMonde);
                    sf::Vector2f mousePos = window.mapPixelToCoords(mouseBtn->position);

                    bool clicSurAvion = false;
                    {
                        std::lock_guard<std::mutex> lock(mutexFlotte);
                        for (auto avion : flotte) {
                            if (avion == nullptr || avion->getEtat() == EtatAvion::TERMINE) continue;
                            sf::Vector2f posAvion = worldToScreen(avion->getPosition());

                            // Distance de clic
                            float dx = mousePos.x - posAvion.x;
                            float dy = mousePos.y - posAvion.y;
                            float dist = std::sqrt(dx * dx + dy * dy);

                            if (dist < (30.0f * niveauZoomActuel)) {
                                avionSelectionne = avion;
                                clicSurAvion = true;
                                break;
                            }
                        }
                    }

                    if (!clicSurAvion) {
                        avionSelectionne = nullptr;
                        if (aeroportVue != nullptr) {
                            aeroportVue = nullptr;
                            vueMonde = window.getDefaultView();
                            niveauZoomActuel = 1.0f;
                            if (hasTextureMap) adapterFondFenetre(spriteFond, textureCarte);
                        }
                        else {
                            for (auto aero : listeAeroports) {
                                sf::Vector2f posAero = worldToScreen(aero->position);
                                float dx = mousePos.x - posAero.x;
                                float dy = mousePos.y - posAero.y;
                                float dist = std::sqrt(dx * dx + dy * dy);

                                if (dist < 50.0f) {
                                    aeroportVue = aero;
                                    vueMonde.setCenter(posAero);
                                    niveauZoomActuel = 0.2f;
                                    // CORRECTION SFML 3 : Accolades obligatoires
                                    vueMonde.setSize({
                                        (float)WINDOW_WIDTH * niveauZoomActuel,
                                        (float)WINDOW_HEIGHT * niveauZoomActuel
                                        });
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        window.clear(sf::Color(30, 30, 30));

        // 1. DESSIN FOND
        window.setView(vueDefaut);
        if (hasTextureMap || aeroportVue != nullptr) {
            window.draw(spriteFond);
        }

        // 2. DESSIN MONDE
        window.setView(vueMonde);

        // Dessin Aéroports
        if (aeroportVue == nullptr) {
            for (auto aero : listeAeroports) {
                sf::Vector2f p = worldToScreen(aero->position);

                sf::CircleShape point(5.f);
                point.setFillColor(sf::Color::Red);
                point.setOrigin({ 5.f, 5.f }); // Accolades
                point.setPosition(p);
                window.draw(point);
                float rayonVisuel = aero->rayonControle * ECHELLE;
                sf::CircleShape zone(rayonVisuel);
                zone.setFillColor(sf::Color(255, 0, 0, 30));
                zone.setOutlineColor(sf::Color::Red);
                zone.setOutlineThickness(1.f);
                zone.setOrigin({ rayonVisuel, rayonVisuel });
                zone.setPosition(p);
                window.draw(zone);

                if (hasFont) {
                    sf::Text text(font, aero->nom, 12);
                    // CORRECTION SFML 3 : setPosition({x, y})
                    text.setPosition({ p.x + 10.f, p.y - 10.f });
                    text.setFillColor(sf::Color::White);
                    window.draw(text);
                }
            }
        }

        // Dessin Avions
        {
            std::lock_guard<std::mutex> lock(mutexFlotte);
            for (auto avion : flotte) {
                if (avion == nullptr) continue;
                if (avion->getEtat() == EtatAvion::TERMINE) continue;

                sf::Vector2f screenPos = worldToScreen(avion->getPosition());
                float rayonBase = (aeroportVue == nullptr) ? 6.f : 15.f;

                sf::CircleShape dot(rayonBase);
                dot.setOrigin({ rayonBase, rayonBase }); // Accolades
                dot.setPosition(screenPos);
                dot.setScale({ niveauZoomActuel, niveauZoomActuel }); // Accolades

                if (avion->estEnUrgence()) {
                    dot.setFillColor(sf::Color::Red);
                    dot.setOutlineColor(sf::Color::Yellow);
                    dot.setOutlineThickness(2.f);
                }
                else if (avion == avionSelectionne) {
                    dot.setFillColor(sf::Color::Green);
                }
                else if (avion->getEtat() == EtatAvion::STATIONNE) {
                    dot.setFillColor(sf::Color(100, 100, 100));
                }
                else {
                    dot.setFillColor(sf::Color::Cyan);
                }

                window.draw(dot);

                // Info Bulle
                if (avion == avionSelectionne && hasFont) {
                    sf::Vector2f tailleBox = { 240.f, 120.f };
                    sf::RectangleShape infoBox(tailleBox);
                    infoBox.setFillColor(sf::Color(0, 0, 0, 200));
                    infoBox.setOutlineColor(sf::Color::White);
                    infoBox.setOutlineThickness(1.f);
                    infoBox.setScale({ niveauZoomActuel, niveauZoomActuel });

                    sf::Vector2f offset = { 20.f * niveauZoomActuel, -60.f * niveauZoomActuel };
                    sf::Vector2f boxPos = screenPos + offset;
                    infoBox.setPosition(boxPos);
                    window.draw(infoBox);

                    std::stringstream ss; 
                    ss << "VOL: " << avion->getNom() << "\n";
                    Aeroport* dest = avion->getDestination();
                    ss << "Dest: " << (dest ? dest->nom : "N/A") << "\n";                    
                    ss << "Alt: " << (int)avion->getPosition().getAltitude() << " m\n";
                    ss << "Fuel: " << (int)avion->getCarburant() << " L\n";
                    float vitesseAffichee = 0.f;
                    EtatAvion etat = avion->getEtat();
                    if (etat == EtatAvion::STATIONNE || etat == EtatAvion::EN_ATTENTE_DECOLLAGE ||
                        etat == EtatAvion::EN_ATTENTE_PISTE || etat == EtatAvion::EN_ATTENTE_ATTERRISSAGE) {
                        vitesseAffichee = 0.f;
                    }
                    else if (etat == EtatAvion::ROULE_VERS_PISTE || etat == EtatAvion::ROULE_VERS_PARKING) {
                        vitesseAffichee = avion->getVitesseSol();
                    }
                    else {
                        vitesseAffichee = avion->getVitesse() / 2.f;
                    }

                    ss << "Vit: " << (int)vitesseAffichee << " km/h\n";
                    if (avion->estEnUrgence()) {
                        if (avion->getTypeUrgence() == TypeUrgence::PANNE_MOTEUR) {
                            ss << "Urgence de type : Panne moteur";
                        }
                        else {
                            ss << "Urgence de type : Médicale";
                        }
                    }
                    std::string etatStr = "INCONNU";
                    switch (avion->getEtat()) {
                    case EtatAvion::STATIONNE:              etatStr = "Stationne"; break;
                    case EtatAvion::EN_ATTENTE_DECOLLAGE:   etatStr = "Attente Decollage"; break;
                    case EtatAvion::ROULE_VERS_PISTE:       etatStr = "Roule vers la piste"; break;
                    case EtatAvion::EN_ATTENTE_PISTE:       etatStr = "En attente au seuil de la piste"; break;
                    case EtatAvion::DECOLLAGE:              etatStr = "Decollage"; break;
                    case EtatAvion::EN_ROUTE:               etatStr = "En Croisiere"; break;
                    case EtatAvion::EN_APPROCHE:            etatStr = "Approche"; break;
                    case EtatAvion::EN_ATTENTE_ATTERRISSAGE:etatStr = "Circuit d'Attente"; break;
                    case EtatAvion::ATTERRISSAGE:           etatStr = "Atterrissage"; break;
                    case EtatAvion::ROULE_VERS_PARKING:     etatStr = "Taxi vers Parking"; break;
                    case EtatAvion::TERMINE:                etatStr = "Termine"; break;
                    }
                    ss << "Etat: " << etatStr << "\n";
                    sf::Text text(font, ss.str(), 14);
                    text.setScale({ niveauZoomActuel, niveauZoomActuel });
                    // CORRECTION SFML 3 : Opération dans les accolades
                    text.setPosition({
                        boxPos.x + 10.f * niveauZoomActuel,
                        boxPos.y + 10.f * niveauZoomActuel
                        });
                    text.setFillColor(sf::Color::White);
                    if (avion->estEnUrgence()) text.setFillColor(sf::Color::Red);

                    window.draw(text);
                }
            }
        }
        window.display();
    }
    {
        std::lock_guard<std::mutex> lock(mutexFlotte);

        for (Avion* avion : flotte) {
            if (avion != nullptr) {
                // On passe l'état à TERMINE pour que le thread pilote s'arrête (si ce n'est pas déjà fait)
                avion->setEtat(EtatAvion::TERMINE);

                // On supprime l'objet en mémoire
                delete avion;
            }
        }
        // On vide le vecteur de pointeurs
        flotte.clear();
    }
    return 0;
}