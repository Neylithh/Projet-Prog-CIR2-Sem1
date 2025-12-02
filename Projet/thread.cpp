#include "thread.hpp"
#include <iostream>
#include <chrono>
#include <random>
#define PROBA_URGENCE 1000

void simuler_pause(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void routine_ccr(CCR& ccr) {
    while (true) {
        ccr.gererEspaceAerien();
        simuler_pause(500);
    }
}

void routine_twr(TWR& twr) {
    while (true) {
        simuler_pause(500);

        Avion* avionPret = twr.choisirAvionPourDecollage();

        if (avionPret != nullptr) {
            if (twr.estPisteLibre() && !twr.estUrgenceEnCours()) {

                twr.reserverPiste();

                if (twr.autoriserDecollage(avionPret)) {
                }
                else {
                    twr.libererPiste();
                }
            }
        }
    }
}

void routine_app(APP& app) {
    while (true) {
        app.mettreAJour();
        simuler_pause(500);
    }
}


void routine_avion(Avion& avion, Aeroport& depart, Aeroport& arrivee, CCR& ccr, std::vector<Aeroport*> aeroports) {

    Aeroport* aeroDepart = &depart;
    Aeroport* aeroArrivee = &arrivee;

    APP* appArrivee = aeroArrivee->app;
    TWR* twrArrivee = aeroArrivee->twr;

    float dt = 1.f;

    while (avion.getEtat() != EtatAvion::TERMINE) {

        EtatAvion etat = avion.getEtat();

        if (etat == EtatAvion::ROULE_VERS_PARKING || etat == EtatAvion::ROULE_VERS_PISTE) {
            avion.avancerSol(dt);
        }
        else if (etat != EtatAvion::STATIONNE && etat != EtatAvion::EN_ATTENTE_DECOLLAGE && etat != EtatAvion::EN_ATTENTE_PISTE) {
            avion.avancer(dt);
        }

        if (etat == EtatAvion::EN_APPROCHE) {
            if (avion.getTrajectoire().empty()) {
                bool autorise = appArrivee->demanderAutorisationAtterrissage(&avion);
                if (!autorise) appArrivee->mettreEnAttente(&avion);
            }
        }

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
                    std::cout << "[TWR] " << avion.getNom() << " bloque la piste (Pas de parking). Evacuation des passagers et annulation du vol.\n";
                    twrArrivee->libererPiste();

                    simuler_pause(3000); // temps que les passagers descendent
                    avion.setEtat(EtatAvion::TERMINE);
                }
            }
        }

        else if (etat == EtatAvion::STATIONNE) {

            simuler_pause(3000);
            if (avion.estEnUrgence()) {
                if (avion.getTypeUrgence() == TypeUrgence::PANNE_MOTEUR) {
                    Logger::getInstance().log("MAINTENANCE", "Reparation", "Moteur en cours de reparation sur " + avion.getNom());
                    simuler_pause(5000);
                }
                else if (avion.getTypeUrgence() == TypeUrgence::MEDICAL) {
                    Logger::getInstance().log("MAINTENANCE", "Evacuation", "Passager malade debarque de " + avion.getNom());
                    simuler_pause(2000);
                }
            }
            avion.effectuerMaintenance();

            Aeroport* nouvelleDestination = aeroArrivee;
            do {
                int idx = std::rand() % aeroports.size();
                nouvelleDestination = aeroports[idx];
            } while (nouvelleDestination == aeroArrivee);

            aeroDepart = aeroArrivee;
            aeroArrivee = nouvelleDestination;

            appArrivee = aeroArrivee->app;

            TWR* twrActuelle = aeroDepart->twr;

            avion.setDestination(aeroArrivee); 
            std::cout << "[AVION] " << avion.getNom() << " : Nouveau plan de vol vers " << aeroArrivee->nom << ".\n";

            twrActuelle->enregistrerPourDecollage(&avion);

            while (avion.getEtat() == EtatAvion::EN_ATTENTE_DECOLLAGE) {
                simuler_pause(200);
            }
        }
        else if (etat == EtatAvion::DECOLLAGE) {
            TWR* twrActuelle = aeroDepart->twr;

            if (avion.getPosition().getAltitude() > 2000) {
                twrActuelle->retirerAvionDeDecollage(&avion);

                std::cout << "[AVION] " << avion.getNom() << " quitte la zone et passe en CROISIERE.\n";

                ccr.prendreEnCharge(&avion);

                twrArrivee = aeroArrivee->twr;
                appArrivee = aeroArrivee->app; 
            }
        }

        if (etat == EtatAvion::EN_ROUTE || etat == EtatAvion::EN_APPROCHE) {
            if (!avion.estEnUrgence() && (rand() % PROBA_URGENCE == 0)) {
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
