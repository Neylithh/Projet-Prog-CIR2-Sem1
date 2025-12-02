#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>

#include "avion.hpp"
#include "thread.hpp"

// Vecteur global pour garder une trace des threads (pour join à la fin si besoin)
std::vector<std::thread> threads_infra;

int main() {
    std::srand(static_cast<unsigned int>(time(NULL)));

    std::cout << "===============================================\n";
    std::cout << "   SIMULATEUR CONTROLE AERIEN (MULTI-THREAD)   \n";
    std::cout << "===============================================\n";

    // 1. Création de l'infrastructure
    CCR ccr;

    Aeroport lille("Lille-Lesquin", Position(0, 0, 0), 20000.0f);
    Aeroport paris("Paris-CDG", Position(90000, 50000, 0), 20000.0f);
    Aeroport marseille("Marseille", Position(300000, 70000, 0), 20000.0f);

    // 2. Lancement des Threads d'infrastructure
    threads_infra.emplace_back(routine_ccr, std::ref(ccr));
    threads_infra.emplace_back(routine_twr, std::ref(*lille.twr));
    threads_infra.emplace_back(routine_app, std::ref(*lille.app));
    std::cout << "[INIT] Aeroport " << lille.nom << " operationnel (TWR + APP).\n";
    threads_infra.emplace_back(routine_twr, std::ref(*paris.twr));
    threads_infra.emplace_back(routine_app, std::ref(*paris.app));
    std::cout << "[INIT] Aeroport " << paris.nom << " operationnel (TWR + APP).\n";
    threads_infra.emplace_back(routine_twr, std::ref(*marseille.twr));
    threads_infra.emplace_back(routine_app, std::ref(*marseille.app));
    std::cout << "[INIT] Aeroport " << marseille.nom << " operationnel (TWR + APP).\n";

    // 3. Simulation du Trafic
    std::cout << "[SYSTEM] Lancement du generateur de trafic...\n\n";

    int idAvion = 1;
    std::vector<Avion*> flotte;

    // Liste des aéroports pour tirage aléatoire
    std::vector<Aeroport*> aeroports = { &lille, &paris, &marseille };

    // Générer 5 avions avec des départs/destinations variés
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Pause plus courte

        std::string nom = "AIRFRANCE-" + std::to_string(idAvion++);

        // Tirage aléatoire départ/destination différents
        int idxDepart = std::rand() % aeroports.size();
        int idxDest;
        do {
            idxDest = std::rand() % aeroports.size();
        } while (idxDest == idxDepart);

        Aeroport* depart = aeroports[idxDepart];
        Aeroport* destination = aeroports[idxDest];

        // Position de départ : en l'air, décalée de l'aéroport de départ
        Position posDepart = depart->position;
        posDepart.setPosition(posDepart.getX(), posDepart.getY() - 5000, 10000);

        // Utilisation du constructeur complet
        Avion* nouvelAvion = new Avion(nom, 400.f, 10.f, 10000.f, 2.f, 5000.f, posDepart);
        nouvelAvion->setDestination(destination);

        flotte.push_back(nouvelAvion);

        ccr.prendreEnCharge(nouvelAvion);

        std::thread tPilote(routine_avion, std::ref(*nouvelAvion), std::ref(*depart), std::ref(*destination), std::ref(ccr), std::ref(aeroports));
        tPilote.detach();

        double dist = nouvelAvion->getPosition().distance(destination->position);
    }

    // Attente de la fin des threads d'infrastructure si besoin
    for (auto& t : threads_infra) {
        if (t.joinable()) t.join();
    }

    // Libération mémoire des avions
    for (auto avion : flotte) {
        delete avion;
    }

    return 0;
}
// test