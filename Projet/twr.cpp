#include "avion.hpp"

// ================= TWR =================

// ================= TWR =================

TWR::TWR(const std::vector<Parking>& parkings, Position posPiste, float tempsAtterrisageDecollage)
    : pisteLibre_(true),
    parkings_(parkings),
    posPiste_(posPiste),
    tempsAtterrissageDecollage_(tempsAtterrisageDecollage),
    urgenceEnCours_(false) // <--- AJOUTER CETTE LIGNE OBLIGATOIREMENT
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

// twr.cpp

bool TWR::autoriserAtterrissage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    // FIX: Check if parking is available BEFORE authorizing landing
    // (Unless it's an emergency, in which case they land anyway)
    bool parkingDispo = false;
    for (const auto& p : parkings_) {
        if (!p.estOccupe()) {
            parkingDispo = true;
            break;
        }
    }

    // Only authorize if Piste is Free AND Parking is Available (or Urgency)
    if ((pisteLibre_ && parkingDispo) || avion->estEnUrgence()) {
        pisteLibre_ = false;
        std::cout << "[TWR] Atterrissage AUTORISE pour " << avion->getNom() << " (Piste reservee).\n";
        avion->setEtat(EtatAvion::ATTERRISSAGE);
        return true;
    }

    // Optional: Log why it was refused
    if (!pisteLibre_) {
        // std::cout << "[TWR] Refus atterrissage " << avion->getNom() << ": Piste occupee.\n";
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
    ss << *avion << " bloqué au bloc " << parking->getNom();
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
    // Vérification pour éviter les doublons dans la file
    if (std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion) == filePourDecollage_.end()) {
        filePourDecollage_.push_back(avion);
        avion->setEtat(EtatAvion::EN_ATTENTE_DECOLLAGE);
        std::cout << "[TWR] " << avion->getNom() << " s'enregistre pour le decollage (Position dans la file: " << filePourDecollage_.size() << ").\n";
    }
}

// --- CORRECTION MAJEURE ICI ---
Avion* TWR::choisirAvionPourDecollage() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    if (filePourDecollage_.empty()) return nullptr;

    // 1. Vérifier si un avion attend DÉJÀ au seuil de piste (Priorité max)
    for (Avion* avion : filePourDecollage_) {
        if (avion->getEtat() == EtatAvion::EN_ATTENTE_PISTE) {
            return avion; // On retourne celui-ci pour que routine_twr autorise le décollage
        }
    }

    // 2. Vérifier si un avion est EN TRAIN de rouler vers la piste
    for (Avion* avion : filePourDecollage_) {
        if (avion->getEtat() == EtatAvion::ROULE_VERS_PISTE) {
            // S'il y en a un qui roule, ON BLOQUE tout départ du parking.
            // On retourne nullptr car aucun avion n'est prêt à décoller immédiatement, 
            // et on ne veut pas en lancer un nouveau.
            return nullptr;
        }
    }

    // 3. Si personne n'est au seuil et personne ne roule, on sort un avion du parking
    Avion* prioritaire = nullptr;
    double maxDistance = -1.0;

    for (Avion* avion : filePourDecollage_) {
        // On ne regarde que ceux qui sont statiques au parking
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

    // Si on a trouvé un candidat au parking, on le fait bouger
    if (prioritaire) {
        std::vector<Position> cheminRoulage;
        cheminRoulage.push_back(posPiste_);
        prioritaire->setTrajectoire(cheminRoulage);

        prioritaire->setEtat(EtatAvion::ROULE_VERS_PISTE);
        std::cout << "[TWR] " << prioritaire->getNom() << " quitte le parking vers la piste (Distance: " << (int)maxDistance << "m).\n";

        // On retourne nullptr car il n'est pas encore prêt à décoller (il doit rouler d'abord)
        return nullptr;
    }

    return nullptr;
}

bool TWR::autoriserDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    if (urgenceEnCours_) {
        std::cout << "[TWR] Decollage refuse pour " << avion->getNom() << " (Priorite à l'urgence).\n";
        return false;
    }

    if (avion->getEtat() == EtatAvion::EN_ATTENTE_PISTE) {
        std::cout << ">>> [TWR] Decollage AUTORISE pour " << avion->getNom() << ". Bon vol !\n";

        // Trajectoire de montée (tout droit, monte à 3000m)
        std::vector<Position> trajMontee;
        Position actuelle = avion->getPosition();
        trajMontee.push_back(Position(actuelle.getX(), actuelle.getY() + 10000, 5000));

        avion->setTrajectoire(trajMontee);
        avion->setEtat(EtatAvion::DECOLLAGE);

        std::stringstream ss;
        ss << "Décollage immédiat piste " << (int)posPiste_.getX() << " pour " << *avion;
        Logger::getInstance().log("TWR", "Decollage", ss.str());

        return true;
    }
    return false;
}

void TWR::retirerAvionDeDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);

    auto it = std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion);

    // VERIFICATION ESSENTIELLE
    if (it != filePourDecollage_.end()) {
        filePourDecollage_.erase(it);
        // Ne libérez la piste que si l'avion y était vraiment
        pisteLibre_ = true;
        std::cout << "[TWR] Piste liberee apres le decollage de " << avion->getNom() << ".\n";
    }
    // Si l'avion n'est pas trouvé, on ne fait rien (évite le crash)
}

void TWR::setUrgenceEnCours(bool statut) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    urgenceEnCours_ = statut;
}

bool TWR::estUrgenceEnCours() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    return urgenceEnCours_;
}