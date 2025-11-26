#include "thread.hpp"
#include <iostream>

void simuler_pause(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ================= THREAD TWR =================
void routine_twr(TWR& twr) {
    while (true) {
        simuler_pause(500); // Check toutes les 0.5s

        // Gestion des décollages
        if (twr.estPisteLibre()) {
            Avion* avionPrioritaire = twr.choisirAvionPourDecollage();

            if (avionPrioritaire != nullptr) {
                // On vérifie si l'avion est prêt (a fini son roulage vers la piste)
                // Note: Dans une vraie simu, il faudrait vérifier la position exacte
                if (avionPrioritaire->getEtat() == EtatAvion::EN_ATTENTE) {
                    twr.autoriserDecollage(avionPrioritaire);
                }
            }
        }
    }
}

// ================= THREAD APP =================
void routine_app(APP& app) {
    // CORRECTION : Ajout de la boucle infinie, sinon le thread s'arrête tout de suite
    while (true) {
        app.mettreAJour();
        simuler_pause(200); // Mise à jour de l'APP toutes les 200ms
    }
}

// ================= THREAD AVION =================
void routine_avion(Avion& avion, APP& app, TWR& twr) {

    // Petite pause initiale pour décaler les démarrages
    simuler_pause(rand() % 1000);

    while (true) {

        EtatAvion etat = avion.getEtat();

        // 1. Mise à jour physique (Sauf si stationné)
        if (etat != EtatAvion::STATIONNE) {
            // dt = 0.5f signifie qu'on avance de 0.5 secondes de simu à chaque tick
            avion.avancer(0.5f);
        }

        // 2. Logique d'état

        // --- ARRIVEE DANS LA ZONE ---
        if (etat == EtatAvion::EN_ROUTE) {
            // Si on approche de la zone (X < 25000) ET qu'on n'a pas encore une trajectoire complexe
            // (planDeVol1 dans main a une taille de 1, donc < 2 est le bon trigger)
            if (avion.getPosition().getX() < 25000 && avion.getTrajectoire().size() <= 1) {
                app.ajouterAvion(&avion);
                app.assignerTrajectoire(&avion);
            }
        }

        // --- APPROCHE ---
        else if (etat == EtatAvion::EN_APPROCHE) {
            // Si on a fini la trajectoire d'approche, on demande à se poser
            if (avion.getTrajectoire().empty()) {
                bool autorise = app.demanderAutorisationAtterrissage(&avion);
                if (!autorise) {
                    // Si refusé, on attend un peu avant de redemander
                    simuler_pause(1000);
                }
            }
        }

        // --- ATTERRISSAGE ---
        else if (etat == EtatAvion::ATTERRISSAGE) {
            // Si altitude proche de 0
            if (avion.getPosition().getAltitude() <= 5.0f) {
                std::cout << ">>> " << avion.getNom() << " TOUCHDOWN !\n";
                // Force Z à 0
                Position p = avion.getPosition();
                avion.setPosition(Position(p.getX(), p.getY(), 0));

                Parking* leParking = twr.attribuerParking(&avion);
                if (leParking != nullptr) {
                    // Créer trajectoire vers parking (simulée simple)
                    std::vector<Position> versParking = { Position(100, 100, 0) }; // Coordonnées parking fictives
                    avion.setTrajectoire(versParking);
                    twr.gererRoulage(&avion, leParking);

                    // IMPORTANT : On libère la piste APRÈS avoir dégagé (ici simulé immédiatement pour simplifier)
                    twr.libererPiste();
                }
                else {
                    // Pas de parking dispo, on attend sur la piste (bloque tout) ou on despawn
                    std::cout << ">>> " << avion.getNom() << " Bloqué sur piste (Pas de parking).\n";
                    twr.libererPiste(); // On libère quand même pour pas casser la simu
                }
            }
        }

        // --- ROULAGE (Vers Parking) ---
        else if (etat == EtatAvion::ROULE && avion.getParking() != nullptr) {
            if (avion.getTrajectoire().empty()) {
                simuler_pause(500);
                avion.setEtat(EtatAvion::STATIONNE);
                std::cout << ">>> " << avion.getNom() << " MOTEURS COUPES au parking "
                    << avion.getParking()->getNom() << ".\n";
            }
        }

        // --- STATIONNEMENT & REDECOLLAGE ---
        else if (etat == EtatAvion::STATIONNE) {
            simuler_pause(5000); // Escale de 5 secondes réelles

            // Demande de départ
            twr.enregistrerPourDecollage(&avion);

            // Libère parking
            if (avion.getParking()) {
                avion.getParking()->liberer();
                // On garde la ref du parking un instant pour la logique de priorité TWR si besoin
                // Mais ici on le set à nullptr
                avion.setParking(nullptr);
            }

            // Roulage vers la piste (Trajectoire fictive)
            std::vector<Position> versPiste = { Position(0,0,0) };
            avion.setTrajectoire(versPiste);
            // On passe en EN_ATTENTE (roulage vers seuil de piste)
            // La TWR passera l'état à DECOLLAGE quand la piste sera libre
            avion.setEtat(EtatAvion::EN_ATTENTE);

            std::cout << "--- " << avion.getNom() << " commence le roulage vers la piste.\n";
        }

        // --- DECOLLAGE ---
        else if (etat == EtatAvion::DECOLLAGE) {
            // Si on vient de passer en décollage et qu'on a pas de traj de montée
            if (avion.getTrajectoire().empty() || avion.getPosition().getAltitude() < 100) {
                std::vector<Position> montee = { Position(40000, 0, 10000) }; // On repart loin
                avion.setTrajectoire(montee);
            }

            // Si on est assez haut/loin
            if (avion.getPosition().getAltitude() > 5000) {
                twr.retirerAvionDeDecollage(&avion);

                // RESET pour boucle infinie du scénario
                std::cout << ">>> " << avion.getNom() << " quitte la zone. (Reset Scenario)\n";

                // On replace l'avion au début
                avion.setPosition(Position(40000 + (rand() % 5000), 0, 10000));
                std::vector<Position> retour = { Position(0,0,0) };
                avion.setTrajectoire(retour);
                avion.setEtat(EtatAvion::EN_ROUTE);

                // Pause avant de revenir
                simuler_pause(2000);
            }
        }

        simuler_pause(100); // Tick rate de l'avion (10Hz)
    }
}