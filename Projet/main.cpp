#include "avion.hpp"
#include "thread.cpp"

main() {
    // A. Préparation du Monde
    std::vector<Parking> parkings;
    parkings.push_back(Parking("P1", 100));
    TWR tour(parkings);
    APP approche(&tour);

    // B. Création de l'Avion (Données)
    Position posInitiale(10000, 10000, 5000);
    std::vector<Position> planDeVol; // Remplis-le si besoin
    Avion monAvion("AFR001", 250.0f, 5000.0f, 10.0f, posInitiale, planDeVol);

    // C. CRÉATION DU THREAD (La syntaxe pure)
    // ---------------------------------------------------------
    // std::thread  nom_variable ( fonction,      ref(arg1),    ref(arg2) );
    std::thread pilote1(routine_avion, std::ref(monAvion), std::ref(approche), std::ref(tour));
    // ---------------------------------------------------------

    // D. Le main fait autre chose (ex: Affichage SFML ou boucle d'attente)
    std::cout << "Simulation lancée..." << std::endl;

    // E. Fin propre (Synchronisation)
    if (pilote1.joinable()) {
        pilote1.join();
    }

    return 0;

}

