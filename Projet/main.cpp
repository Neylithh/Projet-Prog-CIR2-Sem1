#include "avion.hpp"
#include "simulation.cpp"
#include <thread>
#include <chrono>

void routine_avion(Avion& avion, APP& app, TWR& twr) {

    while (avion.getEtat() != EtatAvion::STATIONNE) { // Tant que l'avion n'est pas garé

        avion.avancer(0.5f); // On avance un peu

        if (avion.getEtat() == EtatAvion::EN_ROUTE) { // Si l'avion est en route 
            if (avion.getPosition().getX() < 1000) {  // Si on est à moins de 1000m de l'APP
                app.ajouterAvion(&avion); // On ajoute l'avion à l'APP et on chnage l'etat de l'avion en EN_APPROCHE
            }
        }

        if (avion.getEtat() == EtatAvion::EN_APPROCHE) { // Si l'avion est en approche
            if (avion.getPosition().getX() < 1000)  // Si on est à moins de 1000m de l'APP
                app.demanderAutorisationAtterrissage(&avion); // On demande l'autorisation pour atterrir, si il a l'autorisation on change l'etat de l'avion en ATTERRISSAGE sinon on le met EN_ATTENTE
            }
        }

        if (avion.getEtat() == EtatAvion::EN_ATTENTE) { // Si l'avion est en attente
            if (avion.getPosition().getX() < 1000) { // Si on est à moins de 1000m de l'APP
                app.demanderAutorisationAtterrissage(&avion); // On demande l'autorisation pour atterrir, si il a l'autorisation on change l'etat de l'avion en ATTERRISSAGE sinon ça ne fait rien
            }
        }

        if (avion.getEtat() == EtatAvion::ATTERRISSAGE) { // Si l'avion est en atterrissage
            if (avion.getPosition().getAltitude() <= 0) { // Si on touche le sol

                std::cout << ">>> " << avion.getNom() << " a touché le sol !\n"; // 

                twr.libererPiste(); // Liberer la pist pour le prochain avion (sinon ça va bloquer la piste)

                Parking* leParking = twr.attribuerParking(&avion); // Demande ou se garer

                if (leParking != nullptr) { // change l'etat en ROULE
                    twr.gererRoulage(&avion, leParking);
                }
                else { // Si pas de parking, attendre sur la route
                    std::cout << "ERREUR: Pas de parking pour " << avion.getNom() << "\n";
                }
            }
        }

        if (avion.getEtat() == EtatAvion::ROULE) {
            if (avion.getTrajectoire().empty()) {
                // Pour le réalisme, on peut simuler une petite pause "manoeuvre"
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // On coupe le contact
                avion.setEtat(EtatAvion::STATIONNE);
                std::cout << ">>> " << avion.getNom() << " est garé au parking "
                    << avion.getParking()->getNom() << ". Fin de service.\n";
                // Ici, la condition du while devient fausse -> Le thread s'arrête.
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // On attend un certain temps (poir éviter que le vol dure 1 milliseconde)
    }
}
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
    std::thread     pilote1(routine_avion, std::ref(monAvion), std::ref(approche));
    // ---------------------------------------------------------

    // D. Le main fait autre chose (ex: Affichage SFML ou boucle d'attente)
    std::cout << "Simulation lancée..." << std::endl;

    // E. Fin propre (Synchronisation)
    if (pilote1.joinable()) {
        pilote1.join();
    }

    return 0;

}

