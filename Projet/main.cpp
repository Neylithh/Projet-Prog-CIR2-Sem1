#include "avion.hpp"


int main() {
    std::cout << "========================================\n";
    std::cout << "   TEST DE SIMULATION CONTROLE AERIEN   \n";
    std::cout << "========================================\n";

    // 1. CRÉATION DE L'ENVIRONNEMENT (Le "Monde")
    // On crée 2 parkings pour accueillir nos 2 avions de test
    std::vector<Parking> parkings;
    parkings.push_back(Parking("P-ALPHA", 100)); // Parking proche
    parkings.push_back(Parking("P-BRAVO", 500)); // Parking loin

    // On instancie les contrôleurs
    TWR tour(parkings);
    APP approche(&tour);

    // 2. CRÉATION DES AVIONS (Les "Données")
    // Avion 1 : Loin (30km), arrive vite
    Position pos1(30000, 0, 10000);
    std::vector<Position> planDeVol;
    planDeVol.push_back(Position(0, 0, 0)); // "Va vers le centre"
    // Vitesse 1000.0f pour que ça aille vite à l'écran !
    Avion avion1("AFR001 (Air France)", 1000.0f, 5000.0f, 10.0f, pos1, planDeVol);

    // Avion 2 : Plus proche (22km), arrive aussi vite
    Position pos2(22000, 5000, 8000);
    Avion avion2("BAW404 (British Airways)", 1000.0f, 5000.0f, 10.0f, pos2, planDeVol);

    // 3. LANCEMENT DES THREADS (Les "Pilotes")
    std::cout << "[MAIN] Lancement des threads...\n";

    std::thread t1(routine_avion, std::ref(avion1), std::ref(approche), std::ref(tour));
    std::thread t2(routine_avion, std::ref(avion2), std::ref(approche), std::ref(tour));

    // 4. OBSERVATION (Optionnel : Boucle d'affichage du statut)
    // On regarde ce qui se passe tant que les avions ne sont pas garés
    while (avion1.getEtat() != EtatAvion::STATIONNE ||
        avion2.getEtat() != EtatAvion::STATIONNE) {

        // On affiche juste un petit point pour montrer que le main est vivant
        // (Les logs détaillés sont faits par les cout dans tes classes)
        // std::cout << "."; 
        // std::cout.flush();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 5. NETTOYAGE
    std::cout << "\n[MAIN] Tous les avions semblent garés. Fin des threads.\n";

    if (t1.joinable()) t1.join();
    if (t2.joinable()) t2.join();

    std::cout << "========================================\n";
    std::cout << "        TEST REUSSI AVEC SUCCES         \n";
    std::cout << "========================================\n";

    return 0;
}

