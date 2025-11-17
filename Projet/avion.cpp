#include "avion.hpp"


Avion::Avion(std::string n, float v, float c, float conso,
    Position pos, const std::vector<Position>& traj)
    : nom_(std::move(n)), vitesse_(v), carburant_(c), conso_(conso),
    pos_(pos), trajectoire_(traj), etat_(EtatAvion::EN_ROUTE)
{
}

std::string Avion::getNom() const { return nom_; }
float Avion::getVitesse() const { return vitesse_; }
float Avion::getCarburant() const { return carburant_; }
float Avion::getConsommation() const { return conso_; }
Position Avion::getPosition() const { return pos_; }
const std::vector<Position>& Avion::getTrajectoire() const { return trajectoire_; }
EtatAvion Avion::getEtat() const { return etat_; }

void Avion::setPosition(const Position& p) {
    pos_ = p;
}

void Avion::setTrajectoire(const std::vector<Position>& traj) {
    trajectoire_ = traj;
}

void Avion::setEtat(EtatAvion e) {
    etat_ = e;
}

void Avion::avancer(float temps) {
    if (trajectoire_.empty()) {
        return;
    }
}