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

void Avion::avancer(float dt) {
    if (trajectoire_.empty())
        return;
    Position cible = trajectoire_.front();
    float dx = cible.getX() - pos_.getX();
    float dy = cible.getY() - pos_.getY();
    float dz = cible.getAltitude() - pos_.getAltitude();
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist < 1.0f) {
        trajectoire_.erase(trajectoire_.begin());
        return;
    }

    float nx = dx / dist;
    float ny = dy / dist;
    float nz = dz / dist;
    pos_.setPosition(
        pos_.getX() + nx * vitesse_ * dt,
        pos_.getY() + ny * vitesse_ * dt,
        pos_.getAltitude() + nz * vitesse_ * dt
    );
    carburant_ -= conso_ * dt;
    if (carburant_ < 0) carburant_ = 0;
}




Parking::Parking(const std::string& nom, int distance)
    : nom(nom), distancePiste(distance), occupe(false) {
}

const std::string& Parking::getNom() const {
    return nom;
}

int Parking::getDistancePiste() const {
    return distancePiste;
}

bool Parking::estOccupe() const {
    return occupe;
}

void Parking::occuper() {
    occupe = true;
}

void Parking::liberer() {
    occupe = false;
}
};