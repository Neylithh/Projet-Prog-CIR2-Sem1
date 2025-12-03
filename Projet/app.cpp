#include "avion.hpp"

APP::APP(TWR* tour) : twr_(tour) {}

size_t APP::getNombreAvionsDansZone() const { 
    return avionsDansZone_.size(); 
}

void APP::ajouterAvion(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);
    if (std::find(avionsDansZone_.begin(), avionsDansZone_.end(), avion) == avionsDansZone_.end()) {
        avionsDansZone_.push_back(avion);
        std::cout << "[APP] " << avion->getNom() << " entre dans la zone d'approche.\n";
    }
    std::stringstream ss;
    ss << "L'avion " << *avion << " est pris en charge par l'APP";
    Logger::getInstance().log("APP", "Prise en charge", ss.str());
}

void APP::assignerTrajectoireApproche(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);
    if (!twr_) return;

    Position refPiste = twr_->getPositionPiste();

    float px = refPiste.getX();
    float py = refPiste.getY();

    std::vector<Position> traj = {
        Position(px, py + 3000, 2000),
        Position(px, py + 1000, 1000),
        Position(px, py + 500, 500)
    };

    avion->setTrajectoire(traj);
    avion->setEtat(EtatAvion::EN_APPROCHE);
    std::cout << "[APP] Trajectoire d'approche transmise a " << avion->getNom() << ".\n";
}

void APP::mettreEnAttente(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);

    avion->setEtat(EtatAvion::EN_ATTENTE_ATTERRISSAGE);
    fileAttenteAtterrissage_.push(avion);


    std::vector<Position> cercle;
    Position centre = twr_->getPositionPiste();
    float rayon = avion->getDestination()->rayonControle/2.f;
    float altitudeAttente = 2000.0f; 

    for (int tour = 0; tour < 5; ++tour) {
        for (int angleDeg = 0; angleDeg < 360; angleDeg += 10) {
            float angleRad = angleDeg * (3.14159f / 180.0f);
            float x = centre.getX() + rayon * std::cos(angleRad);
            float y = centre.getY() + rayon * std::sin(angleRad);
            cercle.push_back(Position(x, y, altitudeAttente));
        }
    }

    avion->setTrajectoire(cercle);

    Logger::getInstance().log("APP", "Mise en attente (atterrissage refusé)", "Avion " + avion->getNom() + " est en attente d'atterrissage.");
    std::cout << "[APP] " << avion->getNom() << " entre en circuit d'attente.\n";
}

bool APP::demanderAutorisationAtterrissage(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);
    if (!twr_) return false;

    if (twr_->autoriserAtterrissage(avion)) {
        std::vector<Position> finale;
        finale.push_back(twr_->getPositionPiste());
        avion->setTrajectoire(finale);

        auto it = std::find(avionsDansZone_.begin(), avionsDansZone_.end(), avion);
        if (it != avionsDansZone_.end()) {
            avionsDansZone_.erase(it);
        }

        std::stringstream ss;
        ss << "Autorisation d'atterrir pour " << *avion;
        Logger::getInstance().log("APP", "Autorisation atterrissage", ss.str());

        return true;
    }
    return false;
}

void APP::mettreAJour() {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);

    for (Avion* avion : avionsDansZone_) {
        if (avion->estEnUrgence() && avion->getEtat() == EtatAvion::EN_ATTENTE_ATTERRISSAGE) {

            if (demanderAutorisationAtterrissage(avion)) {
                std::cout << "[APP] URGENCE - PRIORITE D'ATTERRISSAGE ACCORDEE a " << avion->getNom() << " - Il double la file d'attente.\n";
                return;
            }
        }
    }

    if (!fileAttenteAtterrissage_.empty()) {
        Avion* avionEnAttente = fileAttenteAtterrissage_.front();

        if (avionEnAttente->getEtat() != EtatAvion::EN_ATTENTE_ATTERRISSAGE) {
            fileAttenteAtterrissage_.pop();
            return;
        }

        if (twr_->estUrgenceEnCours()) return;

        if (demanderAutorisationAtterrissage(avionEnAttente)) {
            fileAttenteAtterrissage_.pop();
            std::cout << "[APP] " << avionEnAttente->getNom() << " n'est plus pris en charge par l'APP. Atterrissage en cours.\n";
        }
    }

    for (Avion* avion : avionsDansZone_) {
        if (avion->estEnUrgence() && !twr_->estUrgenceEnCours()) {
            if (avion->getEtat() != EtatAvion::ATTERRISSAGE && avion->getEtat() != EtatAvion::EN_APPROCHE) {
                gererUrgence(avion);
            }
        }
    }
}

void APP::gererUrgence(Avion* avion) {
    twr_->setUrgenceEnCours(true);

    std::string typeTxt = "INCONNU";
    switch (avion->getTypeUrgence()) {
    case TypeUrgence::PANNE_MOTEUR: typeTxt = "PANNE MOTEUR"; break;
    case TypeUrgence::MEDICAL: typeTxt = "MEDICAL"; break;
    case TypeUrgence::CARBURANT: typeTxt = "CARBURANT"; break;
    default: break;
    }
    std::cout << "[APP] URGENCE TYPE : " << typeTxt << " pour " << avion->getNom() << ". Priorite absolue.\n";

    Position posPiste = twr_->getPositionPiste();

    std::vector<Position> trajectoireDirecte;


    trajectoireDirecte.push_back(Position(posPiste.getX(), posPiste.getY(), 1000)); 
    trajectoireDirecte.push_back(posPiste);

    avion->setTrajectoire(trajectoireDirecte);
    avion->setEtat(EtatAvion::EN_APPROCHE);

    std::cout << "[APP] Trajectoire directe d'urgence transmise a " << avion->getNom() << ".\n";
}
