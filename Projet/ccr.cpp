#include "avion.hpp"

// ================= CCR =================
CCR::CCR() {}

void CCR::prendreEnCharge(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexCCR_);

    avionsEnCroisiere_.push_back(avion);
    avion->setEtat(EtatAvion::EN_ROUTE);

    // --- CORRECTION : ON DONNE UNE TRAJECTOIRE VERS LA DESTINATION ---
    if (avion->getDestination()) {
        std::vector<Position> routeEnRoute;
        // On lui dit d'aller tout droit vers la position de l'aéroport de destination
        routeEnRoute.push_back(avion->getDestination()->position);
        avion->setTrajectoire(routeEnRoute);
    }
    // -----------------------------------------------------------------

    std::cout << "[CCR] CCR prend en charge " << avion->getNom()
        << ". Route vers "
        << (avion->getDestination() ? avion->getDestination()->nom : "N/A") << " transmise.\n";

    std::stringstream ss;
    ss << "Nouvelle cible radar : " << *avion << " vers " << (avion->getDestination() ? avion->getDestination()->nom : "Inconnu");
    Logger::getInstance().log("CCR", "PriseEnCharge", ss.str());
}


void CCR::transfererVersApproche(Avion* avion, APP* appCible) {
    std::cout << "[CCR] Ne prend plus en charge " << avion->getNom()
        << " et transmet vers l'APP de " << (avion->getDestination() ? avion->getDestination()->nom : "inconnu") << ".\n";

    appCible->ajouterAvion(avion);
    avion->setEtat(EtatAvion::EN_APPROCHE);

    if (avion->estEnUrgence()) {
        appCible->gererUrgence(avion);
    }
    else {
        appCible->assignerTrajectoireApproche(avion);
    }
    std::stringstream ss;
    ss << "Transfert de " << *avion << " vers l'approche locale";
    Logger::getInstance().log("CCR", "Hand-Off", ss.str());
}

#include <sstream> // Nécessaire pour les logs

void CCR::gererEspaceAerien() {
    std::lock_guard<std::mutex> lock(mutexCCR_);

    // --- 1. NOUVEAU : ANTI-COLLISION (Sécurité) ---
    // On vérifie la distance entre chaque paire d'avions en croisière
    for (size_t i = 0; i < avionsEnCroisiere_.size(); ++i) {
        for (size_t j = i + 1; j < avionsEnCroisiere_.size(); ++j) {
            Avion* a1 = avionsEnCroisiere_[i];
            Avion* a2 = avionsEnCroisiere_[j];

            double dist = a1->getPosition().distance(a2->getPosition());

            // Si moins de 5km (5000m) de séparation verticale/horizontale
            if (dist < 5000.0) {
                // LOG DE L'INCIDENT
                std::stringstream ss;
                ss << "ALERTE SEPARATION : " << a1->getNom() << " <-> " << a2->getNom() << " (" << (int)dist << "m)";
                Logger::getInstance().log("CCR", "ConflictAlert", ss.str());

                std::cout << "!!! [CCR] ALERTE COLLISION IMMINENTE : " << a1->getNom() << " / " << a2->getNom() << "\n";

                // Action corrective : On fait monter le premier avion de 500m
                Position p = a1->getPosition();
                a1->setPosition(Position(p.getX(), p.getY(), p.getAltitude() + 500));
            }
        }
    }

    // --- 2. GESTION DES TRANSFERTS (Flux) ---
    for (auto it = avionsEnCroisiere_.begin(); it != avionsEnCroisiere_.end(); ) {
        Avion* avion = *it;
        Aeroport* destination = avion->getDestination();

        if (!destination || avion->getEtat() != EtatAvion::EN_ROUTE) {
            ++it;
            continue;
        }

        double distanceRestante = avion->getPosition().distance(destination->position);

        // CAS A : URGENCE (Priorité Absolue)
        if (avion->estEnUrgence()) {
            APP* appCible = destination->app;

            // LOG
            std::stringstream ss;
            ss << "Transfert PRIORITAIRE (Urgence) de " << *avion << " vers APP";
            Logger::getInstance().log("CCR", "EmergencyHandoff", ss.str());

            std::cout << "[CCR] URGENCE " << avion->getNom() << " transfere en priorite.\n";

            it = avionsEnCroisiere_.erase(it);
            transfererVersApproche(avion, appCible);
            continue;
        }

        // CAS B : ARRIVÉE NORMALE
        if (distanceRestante <= destination->rayonControle) {
            APP* appCible = destination->app;

            // --- NOUVEAU : REGULATION DE FLUX ---
            // On vérifie si l'APP est plein (ex: max 5 avions)
            // Note: Assure-toi d'avoir ajouté getNombreAvionsDansZone() dans APP
            if (appCible->getNombreAvionsDansZone() < 5) {
                // LOG SUCCESS
                std::stringstream ss;
                ss << "Transfert standard de " << *avion << " vers APP";
                Logger::getInstance().log("CCR", "Hand-Off", ss.str());

                it = avionsEnCroisiere_.erase(it);
                transfererVersApproche(avion, appCible);
            }
            else {
                // APP SATURE : REFUS DE TRANSFERT
                // L'avion reste au CCR (il fait un tour dans la boucle for)

                // LOG REGULATION
                std::stringstream ss;
                ss << "Maintien en croisière de " << *avion << " (APP " << destination->nom << " saturé)";
                Logger::getInstance().log("CCR", "Regulation", ss.str());

                std::cout << "[CCR] Maintien " << avion->getNom() << " (APP Sature).\n";

                ++it; // On passe au suivant sans le transférer
            }
        }
        else {
            ++it;
        }
    }
}

// ================= AEROPORT =================
Aeroport::Aeroport(std::string n, Position posAero, float rayon) : nom(n), position(posAero), rayonControle(rayon) {
    Position posPiste(posAero.getX(), posAero.getY(), 0);

    // Add more parkings (e.g., 5 or 6) to avoid congestion
    parkings.push_back(Parking(n + "-G1", Position(posAero.getX() + 100, posAero.getY(), 0)));
    parkings.push_back(Parking(n + "-G2", Position(posAero.getX() + 300, posAero.getY(), 0)));
    parkings.push_back(Parking(n + "-G3", Position(posAero.getX() + 600, posAero.getY(), 0)));
    parkings.push_back(Parking(n + "-G4", Position(posAero.getX() + 800, posAero.getY(), 0))); // New
    parkings.push_back(Parking(n + "-G5", Position(posAero.getX() + 1000, posAero.getY(), 0))); // New

    twr = new TWR(parkings, posPiste, 5000.f);
    app = new APP(twr);
}