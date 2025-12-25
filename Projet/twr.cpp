#include "avion.hpp"

TWR::TWR(const std::vector<Parking>& parkings, Position posPiste, float tempsAtterrisageDecollage)
    : pisteLibre_(true),
    parkings_(parkings),
    posPiste_(posPiste),
    tempsAtterrissageDecollage_(tempsAtterrisageDecollage),
    urgenceEnCours_(false) 
{
}

Position TWR::getPositionPiste() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    return posPiste_;
}

bool TWR::estPisteLibre() const {
    return pisteLibre_;
}

void TWR::libererPiste() {
    pisteLibre_ = true;
}

void TWR::reserverPiste() {
    pisteLibre_ = false;
}


bool TWR::autoriserAtterrissage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    bool parkingDispo = false;
    for (const auto& p : parkings_) {
        if (!p.estOccupe()) {
            parkingDispo = true;
            break;
        }
    }

    if ((pisteLibre_ && parkingDispo) || avion->estEnUrgence()) {
        pisteLibre_ = false;
        std::cout << "[TWR] Atterrissage AUTORISE pour " << avion->getNom() << " (Piste reservee).\n";
        avion->setEtat(EtatAvion::ATTERRISSAGE);
        return true;
    }

    if (!pisteLibre_) {
        std::cout << "[TWR] Refus atterrissage " << avion->getNom() << ": Piste occupee.\n";
    }
    else if (!parkingDispo) {
        std::cout << "[TWR] Refus d'atterrissage " << avion->getNom() << ": Parking COMPLET.\n";
    }

    return false;
}

Parking* TWR::choisirParkingLibre() {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    for (auto& parking : parkings_) {
        if (!parking.estOccupe()) {
            return &parking;
        }
    }
    return nullptr;
}

void TWR::attribuerParking(Avion* avion, Parking* parking) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    avion->setParking(parking);
    std::cout << "[TWR] Parking " << parking->getNom() << " attribue a " << avion->getNom() << ".\n";
    if (avion->estEnUrgence()) {
        urgenceEnCours_ = false;
        std::cout << "[TWR] L'avion en urgence est au parking. Reprise des decollages.\n";
    }
    std::stringstream ss;
    ss << *avion << " bloque au bloc " << parking->getNom();
    Logger::getInstance().log("TWR", "Parking", ss.str());
}

void TWR::gererRoulageVersParking(Avion* avion, Parking* parking) {
    std::vector<Position> cheminRoulage;
    cheminRoulage.push_back(parking->getPosition());

    avion->setTrajectoire(cheminRoulage);
    avion->setEtat(EtatAvion::ROULE_VERS_PARKING);

    std::cout << "[TWR] Roulage vers " << parking->getNom() << " (Piste -> Parking) pour " << avion->getNom() << ".\n";
}

void TWR::enregistrerPourDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion) == filePourDecollage_.end()) {
        filePourDecollage_.push_back(avion);
        avion->setEtat(EtatAvion::EN_ATTENTE_DECOLLAGE);
        std::cout << "[TWR] " << avion->getNom() << " s'enregistre pour le decollage (Position dans la file: " << filePourDecollage_.size() << ").\n";
    }
}

Avion* TWR::choisirAvionPourDecollage() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    if (filePourDecollage_.empty()) return nullptr;

    for (Avion* avion : filePourDecollage_) {
        if (avion->getEtat() == EtatAvion::EN_ATTENTE_PISTE) {
            return avion;
        }
    }

    for (Avion* avion : filePourDecollage_) {
        if (avion->getEtat() == EtatAvion::ROULE_VERS_PISTE) {
            return nullptr;
        }
    }

    Avion* prioritaire = nullptr;
    double maxDistance = -1.0;

    for (Avion* avion : filePourDecollage_) {
        if (avion->getEtat() == EtatAvion::EN_ATTENTE_DECOLLAGE) {
            Parking* parking = avion->getParking();
            if (parking) {
                double distance = parking->getDistancePiste(posPiste_);
                if (distance > maxDistance) {
                    maxDistance = distance;
                    prioritaire = avion;
                }
            }
        }
    }

    if (prioritaire) {
        std::vector<Position> cheminRoulage;
        cheminRoulage.push_back(posPiste_);
        prioritaire->setTrajectoire(cheminRoulage);

        prioritaire->setEtat(EtatAvion::ROULE_VERS_PISTE);
        std::cout << "[TWR] " << prioritaire->getNom() << " quitte le parking vers la piste (Distance: " << (int)maxDistance << "m).\n";

        return nullptr;
    }

    return nullptr;
}

bool TWR::autoriserDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    if (urgenceEnCours_) {
        std::cout << "[TWR] Decollage refuse pour " << avion->getNom() << " (Priorite a l'urgence).\n";
        return false;
    }

    if (avion->getEtat() == EtatAvion::EN_ATTENTE_PISTE) {
        std::cout << "[TWR] Decollage AUTORISE pour " << avion->getNom() << ". Bon vol !\n";

        std::vector<Position> trajMontee;
        Position actuelle = avion->getPosition();
        
        // Modification : Montée progressive vers 3000m sur 20km
        trajMontee.push_back(Position(actuelle.getX(), actuelle.getY() + 20000, 3000));

        avion->setTrajectoire(trajMontee);
        avion->setEtat(EtatAvion::DECOLLAGE);

        std::stringstream ss;
        ss << "Decollage immediat piste " << (int)posPiste_.getX() << " pour " << *avion;
        Logger::getInstance().log("TWR", "Decollage", ss.str());

        return true;
    }
    return false;
}

void TWR::retirerAvionDeDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    auto it = std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion);

    if (it != filePourDecollage_.end()) {
        filePourDecollage_.erase(it);
        pisteLibre_ = true;
        std::cout << "[TWR] Piste liberee apres le decollage de " << avion->getNom() << ".\n";
    }
}

void TWR::setUrgenceEnCours(bool statut) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    urgenceEnCours_ = statut;
}

bool TWR::estUrgenceEnCours() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    return urgenceEnCours_;
}
