#include "avion.hpp"

Avion::Avion(std::string n, float v, float vSol, float c, float conso, float dureeStat, Position pos)
    : nom_(n), vitesse_(v), vitesseSol_(vSol), carburant_(c), conso_(conso),
    dureeStationnement_(dureeStat), pos_(pos), etat_(EtatAvion::STATIONNE),
    parking_(nullptr), destination_(nullptr), typeUrgence_(TypeUrgence::AUCUNE) {
}

std::string Avion::getNom() const {
    return nom_;
}

float Avion::getVitesse() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return vitesse_;
}

float Avion::getVitesseSol() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return vitesseSol_;
}

float Avion::getCarburant() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return carburant_;
}

float Avion::getConsommation() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return conso_;
}

Position Avion::getPosition() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return pos_;
}

EtatAvion Avion::getEtat() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return etat_;
}

Parking* Avion::getParking() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return parking_;
}

Aeroport* Avion::getDestination() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return destination_;
}

float Avion::getDureeStationnement() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return dureeStationnement_;
}

bool Avion::estEnUrgence() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return typeUrgence_ != TypeUrgence::AUCUNE;
}

TypeUrgence Avion::getTypeUrgence() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return typeUrgence_;
}

const std::vector<Position> Avion::getTrajectoire() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return trajectoire_;
}

void Avion::setPosition(const Position& p) {
    std::lock_guard<std::mutex> lock(mtx_);
    pos_ = p;
}

void Avion::setTrajectoire(const std::vector<Position>& traj) {
    std::lock_guard<std::mutex> lock(mtx_);
    trajectoire_ = traj;
}

void Avion::setEtat(EtatAvion e) {
    std::lock_guard<std::mutex> lock(mtx_);
    etat_ = e;
}

void Avion::setParking(Parking* p) {
    std::lock_guard<std::mutex> lock(mtx_);
    parking_ = p;
}

void Avion::setDestination(Aeroport* dest) {
    std::lock_guard<std::mutex> lock(mtx_);
    destination_ = dest;
}

void Avion::avancer(float dt) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (trajectoire_.empty()) return;

    Position cible = trajectoire_.front();
    float dx = cible.getX() - pos_.getX();
    float dy = cible.getY() - pos_.getY();
    float dz = cible.getAltitude() - pos_.getAltitude();
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    float distance_a_parcourir = vitesse_ * dt;

    if (dist <= distance_a_parcourir) {
        pos_ = cible;
        trajectoire_.erase(trajectoire_.begin());
    }
    else {
        float nx = dx / dist;
        float ny = dy / dist;
        float nz = dz / dist;
        pos_.setPosition(
            pos_.getX() + nx * distance_a_parcourir,
            pos_.getY() + ny * distance_a_parcourir,
            pos_.getAltitude() + nz * distance_a_parcourir
        );
    }

    carburant_ -= conso_ * dt;
    if (carburant_ < 0) carburant_ = 0;

    if (carburant_ < 500 && typeUrgence_ == TypeUrgence::AUCUNE) {
        typeUrgence_ = TypeUrgence::CARBURANT;
        std::cout << "[AVION " << nom_ << "] MAYDAY : Urgence CARBURANT declaree !\n";
    }
}

void Avion::avancerSol(float dt) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (trajectoire_.empty()) return;

    Position cible = trajectoire_.front();
    float dx = cible.getX() - pos_.getX();
    float dy = cible.getY() - pos_.getY();
    float dz = cible.getAltitude() - pos_.getAltitude();
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    float distance_a_parcourir = vitesseSol_ * dt;

    if (dist <= distance_a_parcourir) {
        pos_ = cible;
        trajectoire_.erase(trajectoire_.begin());

        if (trajectoire_.empty()) {
            if (etat_ == EtatAvion::ROULE_VERS_PISTE) {
                etat_ = EtatAvion::EN_ATTENTE_PISTE;

                if (parking_) {
                    parking_->liberer();
                    parking_ = nullptr;
                }
                std::cout << "[AVION " << nom_ << "] Arrive a la piste. Etat : EN_ATTENTE_PISTE.\n";
            }
            else if (etat_ == EtatAvion::ROULE_VERS_PARKING) {
                etat_ = EtatAvion::STATIONNE;
                std::cout << "[AVION " << nom_ << "] Arrive au parking "
                    << (parking_ ? parking_->getNom() : "")
                    << ". Etat : STATIONNE. Fin du vol.\n";
            }
        }
    }
    else {
        float nx = dx / dist;
        float ny = dy / dist;
        float nz = dz / dist;
        pos_.setPosition(
            pos_.getX() + nx * distance_a_parcourir,
            pos_.getY() + ny * distance_a_parcourir,
            pos_.getAltitude() + nz * distance_a_parcourir
        );
    }

    float consommationSol = conso_ * 0.05f;
    carburant_ -= consommationSol * dt;
    if (carburant_ < 0) {
        carburant_ = 0;
    }
}

void Avion::declarerUrgence(TypeUrgence type) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (typeUrgence_ == TypeUrgence::AUCUNE) {
        typeUrgence_ = type;
        std::string raison;
        switch (type) {
        case TypeUrgence::PANNE_MOTEUR: raison = "PANNE MOTEUR"; break;
        case TypeUrgence::MEDICAL: raison = "MEDICALE (Passager)"; break;
        case TypeUrgence::CARBURANT: raison = "CARBURANT"; break;
        default: raison = "INCONNUE"; break;
        }
        std::cout << "[AVION " << nom_ << "] MAYDAY : Urgence " << raison << " declaree !\n";
        std::stringstream ss;
        ss << "Urgence déclarée : " << raison << " - Position " << pos_;
        Logger::getInstance().log("AVION", "URGENCE", ss.str());
    }

}

void Avion::effectuerMaintenance() {
    std::lock_guard<std::mutex> lock(mtx_);
    carburant_ = 5000.0f;
    if (typeUrgence_ != TypeUrgence::AUCUNE) {
        std::cout << "[AVION " << nom_ << "] Resolution de l'urgence. Avion operationnel.\n";
        typeUrgence_ = TypeUrgence::AUCUNE;
    }
    else {
        std::cout << "[AVION " << nom_ << "]" << nom_ << " : Remplissage du carburant fini.\n";
    }
}
