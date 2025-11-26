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
    // Lance la boucle de surveillance de l'APP
    app.mettreAJour();
}

// ================= THREAD AVION =================
void routine_avion(Avion& avion, APP& app, TWR& twr) {

    // Petite pause initiale pour décaler les démarrages
    simuler_pause(rand() % 1000);

    while (true) {
        // 1. Mise à jour physique
        if (avion.getEtat() != EtatAvion::STATIONNE) {
            avion.avancer(0.5f); // Avance selon la vitesse
        }

        EtatAvion etat = avion.getEtat();

        // 2. Logique d'état

        // --- ARRIVEE DANS LA ZONE ---
        if (etat == EtatAvion::EN_ROUTE) {
            // Si on approche de la zone (distance arbitraire < 30000)
            if (avion.getPosition().getX() < 25000 && avion.getTrajectoire().size() < 2) {
                app.ajouterAvion(&avion);
                app.assignerTrajectoire(&avion);
            }
        }

        // --- APPROCHE ---
        else if (etat == EtatAvion::EN_APPROCHE) {
            // Si on a fini la trajectoire d'approche, on demande à se poser
            if (avion.getTrajectoire().empty()) {
                app.demanderAutorisationAtterrissage(&avion);
            }
        }

        // --- ATTERRISSAGE ---
        else if (etat == EtatAvion::ATTERRISSAGE) {
            // Si altitude 0 atteinte
            if (avion.getPosition().getAltitude() <= 10.0f) {
                std::cout << ">>> " << avion.getNom() << " TOUCHDOWN !\n";

                Parking* leParking = twr.attribuerParking(&avion);
                if (leParking != nullptr) {
                    // Créer trajectoire vers parking (simulée)
                    std::vector<Position> versParking = { Position(100, 100, 0) };
                    avion.setTrajectoire(versParking);
                    twr.gererRoulage(&avion, leParking);
                    twr.libererPiste(); // Important : libérer la piste APRÈS avoir dégagé
                }
                else {
                    // Pas de parking ? On libère quand même la piste et on despawn ou on attend
                    twr.libererPiste();
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

            if (avion.getParking()) {
                avion.getParking()->liberer();
                avion.setParking(nullptr);
            }

            // Roulage vers la piste
            std::vector<Position> versPiste = { Position(0,0,0) };
            avion.setTrajectoire(versPiste);
            avion.setEtat(EtatAvion::EN_ATTENTE); // On roule vers l'attente

            std::cout << "--- " << avion.getNom() << " demande le roulage pour decollage.\n";
        }

        // --- DECOLLAGE ---
        else if (etat == EtatAvion::DECOLLAGE) {
            if (avion.getTrajectoire().empty()) {
                std::vector<Position> montee = { Position(30000, 0, 10000) }; // On s'éloigne
                avion.setTrajectoire(montee);
            }

            if (avion.getPosition().getAltitude() > 5000) {
                twr.retirerAvionDeDecollage(&avion);
                // Reset pour boucle infinie
                avion.setPosition(Position(40000, 0, 10000));
                std::vector<Position> retour = { Position(0,0,0) };
                avion.setTrajectoire(retour);
                avion.setEtat(EtatAvion::EN_ROUTE);
                std::cout << ">>> " << avion.getNom() << " quitte la frequence (Reset).\n";
            }
        }

        simuler_pause(50); // Tick rate de l'avion
    }
}