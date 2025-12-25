#include "avion.hpp"

CCR::CCR() {}

void CCR::prendreEnCharge(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexCCR_);

    avionsEnCroisiere_.push_back(avion);
    avion->setEtat(EtatAvion::EN_ROUTE);

    if (avion->getDestination()) {
        std::vector<Position> routeEnRoute;
        // Modification : Altitude de croisière réaliste (10000m)
        Position dest = avion->getDestination()->position;
        routeEnRoute.push_back(Position(dest.getX(), dest.getY(), 10000));
        avion->setTrajectoire(routeEnRoute);
    }
    std::cout << "[CCR] CCR prend en charge " << avion->getNom()
        << ". Route vers "
        << (avion->getDestination() ? avion->getDestination()->nom : "N/A") << " transmise.\n";

    std::stringstream ss;
    ss << "Avion " << *avion << " pris en charge par la CCR, en destination de " << avion->getDestination()->nom;
    Logger::getInstance().log("CCR", "Prise en charge", ss.str());
}


void CCR::transfererVersApproche(Avion* avion, APP* appCible) {
    std::cout << "[CCR] Ne prend plus en charge " << avion->getNom() << " et transmet vers l'APP de " << avion->getDestination()->nom << ".\n";

    appCible->ajouterAvion(avion);
    avion->setEtat(EtatAvion::EN_APPROCHE);

    if (avion->estEnUrgence()) {
        appCible->gererUrgence(avion);
    }
    else {
        appCible->assignerTrajectoireApproche(avion);
    }
    std::stringstream ss;
    ss << "Transfert de " << *avion << " vers l'APP";
    Logger::getInstance().log("CCR", "Transfert vers APP", ss.str());
}

void CCR::gererEspaceAerien() {
    std::lock_guard<std::mutex> lock(mutexCCR_);

    for (size_t i = 0; i < avionsEnCroisiere_.size(); ++i) {
        for (size_t j = i + 1; j < avionsEnCroisiere_.size(); ++j) {
            Avion* a1 = avionsEnCroisiere_[i];
            Avion* a2 = avionsEnCroisiere_[j];

            double dist = a1->getPosition().distance(a2->getPosition());

            if (dist < 5000.0) {
                std::stringstream ss;
                ss << "Séparation des avions pour éviter une collision : " << a1->getNom() << " - " << a2->getNom();
                Logger::getInstance().log("CCR", "Collision", ss.str());

                std::cout << "[CCR] Alerte collision : " << a1->getNom() << " / " << a2->getNom() << ". Changement d'altitude pour les deux avions.\n";

                // Avion 1 : Monte (Position + Trajectoire)
                Position p1 = a1->getPosition();
                a1->setPosition(Position(p1.getX(), p1.getY(), p1.getAltitude() + 500));
                
                std::vector<Position> traj1 = a1->getTrajectoire();
                for (auto& pt : traj1) {
                    pt.setPosition(pt.getX(), pt.getY(), pt.getAltitude() + 500);
                }
                a1->setTrajectoire(traj1);

                // Avion 2 : Descend (Position + Trajectoire)
                Position p2 = a2->getPosition();
                a2->setPosition(Position(p2.getX(), p2.getY(), p2.getAltitude() - 500));

                std::vector<Position> traj2 = a2->getTrajectoire();
                for (auto& pt : traj2) {
                    pt.setPosition(pt.getX(), pt.getY(), pt.getAltitude() - 500);
                }
                a2->setTrajectoire(traj2);
            }
        }
    }

    for (auto it = avionsEnCroisiere_.begin(); it != avionsEnCroisiere_.end(); ) {
        Avion* avion = *it;
        Aeroport* destination = avion->getDestination();

        if (!destination || avion->getEtat() != EtatAvion::EN_ROUTE) {
            ++it;
            continue;
        }

        double distanceRestante = avion->getPosition().distance(destination->position);

        if (avion->estEnUrgence()) {
            APP* appCible = destination->app;

            std::stringstream ss;
            ss << "Transfert prioritaire (urgence) de " << *avion << " vers APP";
            Logger::getInstance().log("CCR", "Transfert d'urgence vers APP", ss.str());

            std::cout << "[CCR] URGENCE " << avion->getNom() << " transfert de priorite.\n";

            it = avionsEnCroisiere_.erase(it);
            transfererVersApproche(avion, appCible);
            continue;
        }

        if (distanceRestante <= destination->rayonControle) {
            APP* appCible = destination->app;

            // Modification : Suppression du circuit d'attente CCR.
            // On transfère systématiquement à l'APP quand l'avion est à portée.
            it = avionsEnCroisiere_.erase(it);
            transfererVersApproche(avion, appCible);
        }
        else {
            ++it;
        }
    }
}

Aeroport::Aeroport(std::string n, Position posAero, float rayon) : nom(n), position(posAero), rayonControle(rayon) {
    Position posPiste(posAero.getX(), posAero.getY(), 0);

    parkings.push_back(Parking(n + "-G1", Position(posAero.getX() + 100, posAero.getY(), 0)));
    parkings.push_back(Parking(n + "-G2", Position(posAero.getX() + 300, posAero.getY(), 0)));
    parkings.push_back(Parking(n + "-G3", Position(posAero.getX() + 600, posAero.getY(), 0)));
    parkings.push_back(Parking(n + "-G4", Position(posAero.getX() + 800, posAero.getY(), 0))); // New
    parkings.push_back(Parking(n + "-G5", Position(posAero.getX() + 1000, posAero.getY(), 0))); // New

    twr = new TWR(parkings, posPiste, 5000.f);
    app = new APP(twr);
}
