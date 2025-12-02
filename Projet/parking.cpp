#include "avion.hpp"

// ================= PARKING =================
Parking::Parking(const std::string& nom, Position pos)
    : nom_(nom), pos_(pos), occupe_(false) {
}

const std::string& Parking::getNom() const { return nom_; }

double Parking::getDistancePiste(const Position& posPiste) const {
    return pos_.distance(posPiste);
}

const Position& Parking::getPosition() const {
    return pos_;
}

bool Parking::estOccupe() const { return occupe_; }
void Parking::occuper() { occupe_ = true; }
void Parking::liberer() { occupe_ = false; }