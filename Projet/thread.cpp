#include "thread.hpp"
#include <iostream> // Nécessaire pour les cout

void simuler_pause(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// =========================================================
// C'EST CETTE FONCTION QUI MANQUAIT A L'APPEL (L'erreur VCR001)
// =========================================================
void routine_ccr(CCR& ccr) {
    while (true) {
        // Le cerveau global surveille l'espace aérien en continu
        ccr.gererEspaceAerien();
        simuler_pause(500); // Vérification toutes les 0.5 secondes
    }
}

// --- ROUTINE TWR ---
void routine_twr(TWR& twr) {
    while (true) {
        simuler_pause(500);

        // La tour gère les décollages si la piste est libre
        if (twr.estPisteLibre()) {
            Avion* avion = twr.choisirAvionPourDecollage();
            // On vérifie que l'avion est bien en attente au seuil de piste
            if (avion && avion->getEtat() == EtatAvion::EN_ATTENTE_DECOLLAGE) {
                twr.autoriserDecollage(avion);
            }
        }
    }
}

// --- ROUTINE APP ---
void routine_app(APP& app) {
    while (true) {
        // L'approche met à jour les avions dans sa zone (carburant, trajectoires)
        app.mettreAJour();
        simuler_pause(200);
    }
}

// --- ROUTINE AVION (PILOTE AUTOMATIQUE COMPLET) ---
void routine_avion(Avion& avion, Aeroport& depart, Aeroport& arrivee, CCR& ccr) {

    // On définit la destination dans l'ordinateur de bord
    avion.setDestination(&arrivee);

    // 1. Initialisation au sol : Demande de parking à l'aéroport de départ
    Parking* pDepart = depart.twr->attribuerParking(&avion);
    if (!pDepart) {
        // Si l'aéroport est plein à craquer, le vol est annulé
        // (Peu probable au démarrage mais bonne pratique)
        avion.setEtat(EtatAvion::TERMINE);
        return;
    }

    // Simulation de l'embarquement passagers
    simuler_pause(2000);

    while (avion.getEtat() != EtatAvion::TERMINE) {
        EtatAvion etat = avion.getEtat();

        // --- PHASE 1 : AU SOL (DEPART) ---
        if (etat == EtatAvion::STATIONNE) {
            // Le pilote demande l'autorisation de mise en route
            depart.twr->enregistrerPourDecollage(&avion);

            // On libère le parking
            if (avion.getParking()) {
                avion.getParking()->liberer();
                avion.setParking(nullptr);
            }

            // Roulage vers la piste (Trajectoire fictive simple)
            std::vector<Position> taxi = { depart.position };
            avion.setTrajectoire(taxi);

            // Temps de roulage
            simuler_pause(1000);

            // Arrivée au seuil de piste, prêt au départ
            avion.setEtat(EtatAvion::EN_ATTENTE_DECOLLAGE);
        }

        // --- PHASE 2 : DECOLLAGE ET MONTEE ---
        else if (etat == EtatAvion::DECOLLAGE) {
            // Si on n'a pas de trajectoire de montée, on en crée une (vers 8000m)
            if (avion.getTrajectoire().empty()) {
                Position cible = avion.getPosition();
                cible.setPosition(cible.getX(), cible.getY(), 8000);
                std::vector<Position> montee = { cible };
                avion.setTrajectoire(montee);
            }

            // Une fois une certaine altitude atteinte (5000m), on passe sur la fréquence régionale (CCR)
            if (avion.getPosition().getAltitude() > 5000) {
                // La tour nous oublie
                depart.twr->retirerAvionDeDecollage(&avion);

                // Le CCR nous prend en charge
                ccr.prendreEnCharge(&avion);

                // On met le cap direct sur l'aéroport d'arrivée
                std::vector<Position> route = { arrivee.position };
                avion.setTrajectoire(route);
            }
        }

        // --- PHASE 3 : CROISIERE (Géré par CCR) ---
        else if (etat == EtatAvion::EN_ROUTE) {
            // On calcule la distance restante
            float dist = avion.getPosition().distance(arrivee.position);

            // Si on est à moins de 20km (20000 unités), on contacte l'Approche (APP)
            if (dist < 20000) {
                ccr.transfererVersApproche(&avion, arrivee.app);
                // Note : Cette fonction change l'état de l'avion en EN_APPROCHE
            }
        }

        // --- PHASE 4 : APPROCHE (Géré par APP) ---
        else if (etat == EtatAvion::EN_APPROCHE) {
            // Si on a fini les vecteurs d'approche, on demande la finale (atterrissage)
            if (avion.getTrajectoire().empty()) {
                bool autorise = arrivee.app->demanderAutorisationAtterrissage(&avion);
                if (!autorise) {
                    simuler_pause(1000); // On fait un tour d'attente si refusé
                }
            }
        }

        // --- PHASE 5 : ATTERRISSAGE ET ARRRET ---
        else if (etat == EtatAvion::ATTERRISSAGE) {
            // Si on touche le sol (Altitude proche de 0)
            if (avion.getPosition().getAltitude() <= 10.0f) {

                // On demande un parking à l'arrivée
                Parking* pArrivee = arrivee.twr->attribuerParking(&avion);

                if (pArrivee) {
                    // On libère la piste immédiatement
                    arrivee.twr->libererPiste();
                    arrivee.twr->gererRoulageAuSol(&avion, pArrivee);

                    simuler_pause(1000); // Temps de roulage vers le gate

                    avion.setEtat(EtatAvion::TERMINE);
                }
                else {
                    // Cas critique : piste libérée mais pas de parking (évacuation)
                    arrivee.twr->libererPiste();
                    avion.setEtat(EtatAvion::TERMINE);
                }
            }
        }

        // --- MOTEUR PHYSIQUE ---
        // Tant que le vol n'est pas fini et qu'on n'attend pas au sol
        if (etat != EtatAvion::TERMINE && etat != EtatAvion::EN_ATTENTE_DECOLLAGE && etat != EtatAvion::STATIONNE) {
            // On va plus vite en croisière (x3) pour ne pas attendre 10 ans devant l'écran
            float dt = (etat == EtatAvion::EN_ROUTE) ? 3.0f : 0.8f;
            avion.avancer(dt);
        }

        // Tick rate du pilote (réflexes)
        simuler_pause(100);
    }
}