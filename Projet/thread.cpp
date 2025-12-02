#include "thread.hpp"
#include <iostream>
#include <chrono>
#include <random>


void simuler_pause(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// --- ROUTINE CCR ---
void routine_ccr(CCR& ccr) {
    while (true) {
        ccr.gererEspaceAerien();
        simuler_pause(500);
    }
}

// --- ROUTINE TWR (CORRIGEE) ---
void routine_twr(TWR& twr) {
    while (true) {
        simuler_pause(500);

        Avion* avionPret = twr.choisirAvionPourDecollage();

        if (avionPret != nullptr) {
            // On ne tente le décollage que si la piste est libre ET pas d'urgence
            if (twr.estPisteLibre() && !twr.estUrgenceEnCours()) {

                // 1. On réserve la piste
                twr.reserverPiste();

                // 2. On tente d'autoriser le décollage
                if (twr.autoriserDecollage(avionPret)) {
                    // SUCCES : L'avion passe en état DECOLLAGE, la piste reste réservée 
                    // jusqu'à ce qu'il soit assez haut (géré par retirerAvionDeDecollage)
                }
                else {
                    // ECHEC : Si le décollage est refusé (ex: urgence apparue entre temps),
                    // IL FAUT LIBERER LA PISTE IMMÉDIATEMENT !
                    twr.libererPiste();
                }
            }
        }
    }
}

// --- ROUTINE APP ---
void routine_app(APP& app) {
    while (true) {
        app.mettreAJour();
        simuler_pause(500);
    }
}

// DANS thread.cpp

void routine_avion(Avion& avion, Aeroport& depart, Aeroport& arrivee, CCR& ccr, std::vector<Aeroport*> aeroports) {

    // On utilise des pointeurs dynamiques pour pouvoir échanger départ et arrivée
    Aeroport* aeroDepart = &depart;
    Aeroport* aeroArrivee = &arrivee;

    // On récupère les contrôleurs de l'aéroport d'arrivée actuel
    APP* appArrivee = aeroArrivee->app;
    TWR* twrArrivee = aeroArrivee->twr;

    float dt = 1.f;

    while (avion.getEtat() != EtatAvion::TERMINE) {

        EtatAvion etat = avion.getEtat();

        // --- 1. MOUVEMENT ---
        if (etat == EtatAvion::ROULE_VERS_PARKING || etat == EtatAvion::ROULE_VERS_PISTE) {
            avion.avancerSol(dt);
        }
        else if (etat != EtatAvion::STATIONNE && etat != EtatAvion::EN_ATTENTE_DECOLLAGE && etat != EtatAvion::EN_ATTENTE_PISTE) {
            avion.avancer(dt);
        }

        // --- 2. TRANSITIONS D'ETATS ---

        // A. ARRIVÉE ZONE APPROCHE
        if (etat == EtatAvion::EN_APPROCHE) {
            if (avion.getTrajectoire().empty()) {
                bool autorise = appArrivee->demanderAutorisationAtterrissage(&avion);
                if (!autorise) appArrivee->mettreEnAttente(&avion);
            }
        }

        // B. ATTERRISSAGE
        else if (etat == EtatAvion::ATTERRISSAGE) {
            if (avion.getTrajectoire().empty()) {
                Parking* p = twrArrivee->choisirParkingLibre();

                if (p) {
                    p->occuper();
                    twrArrivee->attribuerParking(&avion, p);
                    twrArrivee->gererRoulageVersParking(&avion, p);
                    twrArrivee->libererPiste();
                }
                else {
                    // --- AJOUT DE SECURITÉ ---
                    // Si on a atterri mais pas de parking, on ne peut pas rester sur la piste !
                    // On libère la piste pour ne pas bloquer tout l'aéroport.
                    std::cout << "[TWR] " << avion.getNom() << " bloque la piste (Pas de parking). Evacuation des passagers et annulation du vol.\n";
                    twrArrivee->libererPiste();

                    // On force l'avion à disparaître ou à attendre hors piste
                    simuler_pause(3000); // temps que les passagers descendent
                    avion.setEtat(EtatAvion::TERMINE);
                }
            }
        }

        // C. STATIONNEMENT -> PRÉPARATION AU RETOUR
        else if (etat == EtatAvion::STATIONNE) {

            simuler_pause(3000);
            if (avion.estEnUrgence()) {
                if (avion.getTypeUrgence() == TypeUrgence::PANNE_MOTEUR) {
                    Logger::getInstance().log("MAINTENANCE", "Reparation", "Moteur en cours de réparation sur " + avion.getNom());
                    simuler_pause(5000);
                }
                else if (avion.getTypeUrgence() == TypeUrgence::MEDICAL) {
                    Logger::getInstance().log("MAINTENANCE", "Evacuation", "Passager malade débarqué de " + avion.getNom());
                    simuler_pause(2000);
                }
            }
            avion.effectuerMaintenance();

            // --- CHANGEMENT DE DESTINATION ICI ---
            Aeroport* nouvelleDestination = aeroArrivee;
            do {
                int idx = std::rand() % aeroports.size();
                nouvelleDestination = aeroports[idx];
            } while (nouvelleDestination == aeroArrivee);

            aeroDepart = aeroArrivee;
            aeroArrivee = nouvelleDestination;

            // Mise à jour des contrôleurs locaux pour la nouvelle destination
            appArrivee = aeroArrivee->app;

            // Note: Pour le décollage, on parle encore à la TWR où on est garé (aeroDepart désormais)
            TWR* twrActuelle = aeroDepart->twr;

            avion.setDestination(aeroArrivee); // On vise le nouvel aéroport
            std::cout << "[AVION] " << avion.getNom() << " : Nouveau plan de vol vers " << aeroArrivee->nom << ".\n";

            // On demande le décollage à la tour actuelle
            twrActuelle->enregistrerPourDecollage(&avion);

            // Attente passive
            while (avion.getEtat() == EtatAvion::EN_ATTENTE_DECOLLAGE) {
                simuler_pause(200);
            }
        }

        // D. DECOLLAGE -> RETOUR EN CROISIERE (BOUCLE)
        // ... Bloc DECOLLAGE ...
        else if (etat == EtatAvion::DECOLLAGE) {
            // On parle à la tour de DEPART (où on vient de décoller)
            TWR* twrActuelle = aeroDepart->twr;

            if (avion.getPosition().getAltitude() > 2000) {
                // C'est ici qu'on retire l'avion de la liste de décollage
                twrActuelle->retirerAvionDeDecollage(&avion);

                std::cout << "[AVION] " << avion.getNom() << " quitte la zone et passe en CROISIERE.\n";

                ccr.prendreEnCharge(&avion);

                // --- CORRECTION IMPORTANTE ---
                // On met à jour le pointeur pour le PROCHAIN atterrissage
                // Sinon, au prochain tour de boucle, on contactera la mauvaise tour
                twrArrivee = aeroArrivee->twr;
                appArrivee = aeroArrivee->app; // On met aussi à jour l'APP par sécurité
                // -----------------------------
            }
        }

        // --- GESTION PANNES ---
        if (etat == EtatAvion::EN_ROUTE || etat == EtatAvion::EN_APPROCHE) {
            if (!avion.estEnUrgence() && (rand() % 1500 == 0)) {
                if (rand() % 2 == 0) {
                    avion.declarerUrgence(TypeUrgence::MEDICAL);
                }
                else {
                    avion.declarerUrgence(TypeUrgence::PANNE_MOTEUR);
                }
            }
        }

        simuler_pause(75);
    }
}