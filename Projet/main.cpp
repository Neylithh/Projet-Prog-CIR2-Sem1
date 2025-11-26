#include "avion.hpp"
#include "thread.hpp"

int main() {
    std::cout << "========================================\n";
    std::cout << "    SIMULATION CONTROLE AERIEN C++      \n";
    std::cout << "========================================\n";

    // 1. Initialisation Environnement
    std::vector<Parking> parkings;
    parkings.push_back(Parking("P-ALPHA", 100));
    parkings.push_back(Parking("P-BRAVO", 500));
    parkings.push_back(Parking("P-CHARLIE", 800));

    TWR tour(parkings);
    APP approche(&tour);

    // 2. Création des Avions
    Position pos1(40000, 0, 10000); // Loin
    std::vector<Position> planDeVol1 = { Position(0, 0, 0) }; // Vers l'aéroport
    Avion avion1("AFR001", 800.0f, 5000.0f, 10.0f, pos1, planDeVol1);

    Position pos2(35000, 5000, 10000);
    std::vector<Position> planDeVol2 = { Position(0, 0, 0) };
    Avion avion2("BAW404", 800.0f, 5000.0f, 10.0f, pos2, planDeVol2);

    // 3. Lancement des Threads
    std::cout << "[MAIN] Lancement des services...\n";

    std::thread tTwr(routine_twr, std::ref(tour));
    std::thread tApp(routine_app, std::ref(approche)); // Thread APP ajouté

    std::cout << "[MAIN] Lancement des pilotes...\n";
    std::thread t1(routine_avion, std::ref(avion1), std::ref(approche), std::ref(tour));
    std::thread t2(routine_avion, std::ref(avion2), std::ref(approche), std::ref(tour));

    // 4. Boucle principale (Observation)
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Affiche la position de l'avion 1 juste pour vérifier qu'il bouge
        // Note: C'est du debug rapide
        Position p = avion1.getPosition();
        std::cout << "[RADAR] " << avion1.getNom()
            << " est en (" << (int)p.getX() << ", "
            << (int)p.getY() << ", "
            << (int)p.getAltitude() << ")\n";
    }

    if (t1.joinable()) t1.join();
    if (t2.joinable()) t2.join();
    if (tTwr.joinable()) tTwr.join();
    if (tApp.joinable()) tApp.join();

    return 0;
}