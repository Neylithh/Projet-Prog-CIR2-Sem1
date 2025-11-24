#include "avion.hpp"

Position::Position(float x, float y, float z)
    : x_(x), y_(y), altitude_(z) {
}
float Position::getX() const { return x_; }
float Position::getY() const { return y_; }
float Position::getAltitude() const { return altitude_; }
void Position::setPosition(float x, float y, float alt) {
    x_ = x;
    y_ = y;
    altitude_ = alt;
}

float Position::distance(const Position& other) const {
    float dx = x_ - other.x_;
    float dy = y_ - other.y_;
    float dz = altitude_ - other.altitude_;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}


Avion::Avion(std::string n, float v, float c, float conso, Position pos, const std::vector<Position>& traj)
    : nom_(n), vitesse_(v), carburant_(c), conso_(conso), pos_(pos), trajectoire_(traj), etat_(EtatAvion::EN_ROUTE) {}

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

TWR::TWR(const std::vector<Parking>& parkings)
    : pisteLibre_(true), parkings_(parkings) {
}

bool TWR::estPisteLibre() const {
    return pisteLibre_;
}

bool TWR::autoriserAtterrissage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (pisteLibre_) {
        pisteLibre_ = false; // On verrouille la piste
        std::cout << "[TWR] Atterrissage autorisé pour " << avion->getNom() << std::endl;
        avion->setEtat(EtatAvion::ATTERRISSAGE);
        return true;
    }
    std::cout << "[TWR] Atterrissage refusé pour " << avion->getNom() << " (Piste occupée)" << std::endl;
    return false;
}

void TWR::libererPiste() {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    pisteLibre_ = true;
    std::cout << "[TWR] La piste a été libérée." << std::endl;
}

Parking* TWR::attribuerParking(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    for (auto& parking : parkings_) {
        if (!parking.estOccupe()) {
            parking.occuper();
            std::cout << "[TWR] Parking " << parking.getNom()
                << " attribué à " << avion->getNom() << std::endl;
            return &parking;
        }
    }
    std::cout << "[TWR] Aucun parking disponible pour " << avion->getNom() << std::endl;
    return nullptr;
}

void TWR::gererRoulage(Avion* avion, Parking* parking) {
    avion->setEtat(EtatAvion::ROULE);
    int distance = parking->getDistancePiste();
    std::cout << "[TWR] ORDRE de roulage donné à " << avion->getNom()
        << ". Distance à la piste : " << distance << " unités." << std::endl;
}

void TWR::enregistrerPourDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    filePourDecollage_.push_back(avion);
    avion->setEtat(EtatAvion::EN_ATTENTE);
    std::cout << "[TWR] " << avion->getNom() << " enregistré dans la file de décollage. Position dans la file: "
        << filePourDecollage_.size() << std::endl;
}

bool TWR::autoriserDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (pisteLibre_) {
        pisteLibre_ = false;
        std::cout << "[TWR] Décollage autorisé pour " << avion->getNom() << " (Piste réservée)." << std::endl;
        avion->setEtat(EtatAvion::DECOLLAGE);
        return true;
    }
    return false;
}

Avion* TWR::choisirAvionPourDecollage() const {
    if (filePourDecollage_.empty()) {
        return nullptr;
    }
    Avion* prioritaire = nullptr;
    int maxDistance = -1; 
    if (!filePourDecollage_.empty()) {
        prioritaire = filePourDecollage_.front();
    }
    return prioritaire;
}

void TWR::retirerAvionDeDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    auto it = std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion);
    if (it != filePourDecollage_.end()) {
        std::cout << "[TWR] " << avion->getNom() << " retiré de la file de décollage." << std::endl;
        filePourDecollage_.erase(it);
    }
    libererPiste();
}