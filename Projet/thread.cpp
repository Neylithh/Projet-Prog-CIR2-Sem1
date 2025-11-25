#include "thread.hpp" 

// Fonction utilitaire pour simuler une pause (utilise std::this_thread::sleep_for)
void simuler_pause(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ====================================================================
// Routine du Thread de la Tour de Contrôle (TWR)
// ====================================================================

void routine_twr(TWR& twr) {
    // La TWR tourne en permanence pour gérer les priorités au décollage et la piste
    while (true) {
        simuler_pause(100); // Temps de traitement cyclique de la TWR

        // Si la piste est libre (vérification thread-safe)
        if (twr.estPisteLibre()) {

            // Choisir l'avion le plus prioritaire au décollage (vérification thread-safe)
            Avion* avionPrioritaire = twr.choisirAvionPourDecollage();

            if (avionPrioritaire != nullptr) {
                // Si l'avion est prêt (en phase de roulage pour décoller, près de la piste)
                if (avionPrioritaire->getEtat() == EtatAvion::ROULE) {

                    // Tenter d'autoriser le décollage (passe l'avion à DECOLLAGE et réserve la piste)
                    if (twr.autoriserDecollage(avionPrioritaire)) {
                        // L'avion va exécuter la phase DECOLLAGE dans son propre thread.
                        std::cout << "[TWR] Autorisation DECOLLAGE accordée à l" << avionPrioritaire->getNom() << "\n";
                    }
                }
            }
        }
    }
}

// ====================================================================
// Routine du Thread de l'Avion (Cycle Complet Aéroportuaire)
// ====================================================================

void routine_avion(Avion& avion, APP& app, TWR& twr) {

    // Le cycle de vie de l'avion continue tant que le thread tourne
    while (true) {

        // L'avion avance s'il a une trajectoire à suivre ET qu'il n'est pas STATIONNE
        if (avion.getEtat() != EtatAvion::STATIONNE && !avion.getTrajectoire().empty()) {
            avion.avancer(0.5f);
        }

        // --- LOGIQUE ARRIVEE ---

        // 1. Entrée dans la zone APP (EN_ROUTE -> EN_APPROCHE)
        if (avion.getEtat() == EtatAvion::EN_ROUTE) {
            if (avion.getPosition().getX() < 1000) {
                app.ajouterAvion(&avion); // Change l'état à EN_APPROCHE
                app.assignerTrajectoire(&avion); // Assignation de la première trajectoire d'approche
            }
        }

        // 2. Demande d'Atterrissage (EN_APPROCHE -> ATTERRISSAGE ou EN_ATTENTE)
        if (avion.getEtat() == EtatAvion::EN_APPROCHE) {
            // Si la trajectoire d'approche est terminée, demande l'autorisation TWR
            if (avion.getTrajectoire().empty())
            {
                // L'APP gère la demande qui interroge la TWR
                app.demanderAutorisationAtterrissage(&avion);
            }
        }

        // 3. Boucle d'Attente (EN_ATTENTE -> ATTERRISSAGE)
        if (avion.getEtat() == EtatAvion::EN_ATTENTE) {
            // Si le circuit d'attente est terminé, on redemande l'autorisation
            if (avion.getTrajectoire().empty()) {
                app.demanderAutorisationAtterrissage(&avion);
            }
        }

        // 4. Atterrissage et Roulage d'arrivée (ATTERRISSAGE -> ROULE)
        if (avion.getEtat() == EtatAvion::ATTERRISSAGE) {
            if (avion.getPosition().getAltitude() <= 0) { // Touché des roues

                std::cout << ">>> " << avion.getNom() << " a touché le sol !\n";

                // L'avion a besoin de libérer la piste pour les autres avions.
                twr.libererPiste();

                Parking* leParking = twr.attribuerParking(&avion); // Demande d'un parking

                if (leParking != nullptr) {
                    // Définir une trajectoire vers le parking (simplifiée)
                    std::vector<Position> trajRoule = { Position(leParking->getDistancePiste(), 0, 0) };
                    avion.setTrajectoire(trajRoule);
                    twr.gererRoulage(&avion, leParking); // Change l'état à ROULE
                }
            }
        }

        // 5. Fin du roulage d'arrivée (ROULE -> STATIONNE)
        if (avion.getEtat() == EtatAvion::ROULE && avion.getParking() != nullptr) {
            if (avion.getTrajectoire().empty()) {

                simuler_pause(500); // Simuler la manoeuvre de parking
                avion.setEtat(EtatAvion::STATIONNE);
                std::cout << ">>> " << avion.getNom() << " est garé au parking "
                    << avion.getParking()->getNom() << ". Prêt pour la phase DEPART.\n";
            }
        }

        // --- LOGIQUE DEPART ---

        // 6. Préparation au départ (STATIONNE -> ROULE)
        if (avion.getEtat() == EtatAvion::STATIONNE) {
            simuler_pause(2000); // Simuler la phase au sol (ravitaillement, passagers)

            // Enregistrement pour le décollage
            twr.enregistrerPourDecollage(&avion); // Change l'état à EN_ATTENTE (file TWR)

            // Libération du parking
            if (avion.getParking()) {
                avion.getParking()->liberer();
                avion.setParking(nullptr); // L'avion n'est plus "occupant" du parking
            }

            // Roulage vers la piste (point d'attente)
            std::vector<Position> trajRoulePiste = { Position(0, 0, 0) };
            avion.setTrajectoire(trajRoulePiste);
            avion.setEtat(EtatAvion::ROULE); // Repasse en ROULE (roulage vers la piste)

            std::cout << "--- " << avion.getNom() << " commence le roulage vers la piste. En attente de l'autorisation TWR. ---\n";
        }

        // 7. Décollage (DECOLLAGE -> EN_ROUTE)
        // Note: L'état DECOLLAGE est donné par la TWR dans routine_twr().
        if (avion.getEtat() == EtatAvion::DECOLLAGE) {

            // Phase d'accélération et d'envol (pendant cette phase, la piste est BLOQUÉE)
            if (avion.getPosition().getAltitude() < 15000) {
                // Assurez-vous d'avoir une trajectoire de montée pour la phase de décollage.
                if (avion.getTrajectoire().empty()) {
                    std::vector<Position> trajDecollage = { Position(1000, 500, 5000), Position(10000, 10000, 15000) };
                    avion.setTrajectoire(trajDecollage);
                }
            }

            // Fin du décollage et sortie de la zone aéroportuaire
            else if (avion.getPosition().getAltitude() >= 15000) {
                twr.retirerAvionDeDecollage(&avion); // Libère la piste et retire de la file (FIN TWR)
                avion.setEtat(EtatAvion::EN_ROUTE); // Redémarre le cycle EN_ROUTE
                std::cout << ">>> " << avion.getNom() << " a atteint son altitude de croisière. Le cycle recommence.\n";
            }
        }


        simuler_pause(50); // Fréquence de rafraîchissement
    }
}