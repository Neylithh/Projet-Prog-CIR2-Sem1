#include "avion.hpp"

// ================= APP =================
APP::APP(TWR* tour) : twr_(tour) {}

size_t APP::getNombreAvionsDansZone() const { 
    return avionsDansZone_.size(); 
}

void APP::ajouterAvion(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);
    // Evite doublons
    if (std::find(avionsDansZone_.begin(), avionsDansZone_.end(), avion) == avionsDansZone_.end()) {
        avionsDansZone_.push_back(avion);
        std::cout << "[APP] " << avion->getNom() << " entre dans la zone d'approche.\n";
    }
    std::stringstream ss;
    ss << "Contact radar confirmé avec " << *avion;
    Logger::getInstance().log("APP", "RadarContact", ss.str());
}

void APP::assignerTrajectoireApproche(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);
    if (!twr_) return; // Sécurité

    // ON RECUPERE LA POS VIA TWR
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
    std::cout << "[APP] Trajectoire d'approche transmise à " << avion->getNom() << ".\n";
}

void APP::mettreEnAttente(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);

    // 1. Changement d'état
    avion->setEtat(EtatAvion::EN_ATTENTE_ATTERRISSAGE);
    fileAttenteAtterrissage_.push(avion);

    // 2. Génération de la trajectoire circulaire (Nouveau)
    // On génère un cercle de rayon 5km autour de la piste
    std::vector<Position> cercle;
    Position centre = twr_->getPositionPiste();
    float rayon = 5000.0f;
    float altitudeAttente = 2000.0f; // On empile à 2000m

    // On crée 5 tours de boucle (suffisant pour attendre)
    for (int tour = 0; tour < 5; ++tour) {
        for (int angleDeg = 0; angleDeg < 360; angleDeg += 10) {
            float angleRad = angleDeg * (3.14159f / 180.0f);
            float x = centre.getX() + rayon * std::cos(angleRad);
            float y = centre.getY() + rayon * std::sin(angleRad);
            cercle.push_back(Position(x, y, altitudeAttente));
        }
    }

    avion->setTrajectoire(cercle);

    // 3. Log
    Logger::getInstance().log("APP", "HoldingPattern",
        "Avion " + avion->getNom() + " entre en circuit d'attente circulaire.");
    std::cout << "[APP] " << avion->getNom() << " entre en circuit d'attente (Trajectoire circulaire générée).\n";
}

// Modifiez la méthode existante pour retirer l'avion de la liste
bool APP::demanderAutorisationAtterrissage(Avion* avion) {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);
    if (!twr_) return false;

    if (twr_->autoriserAtterrissage(avion)) {
        std::vector<Position> finale;
        finale.push_back(twr_->getPositionPiste());
        avion->setTrajectoire(finale);

        // --- CORRECTION CRITIQUE : ON OUBLIE L'AVION ---
        // On retire l'avion de la liste de l'APP car il passe à la TWR
        auto it = std::find(avionsDansZone_.begin(), avionsDansZone_.end(), avion);
        if (it != avionsDansZone_.end()) {
            avionsDansZone_.erase(it);
            // On ne log pas forcément pour ne pas spammer, mais c'est fait.
        }
        // -----------------------------------------------

        std::stringstream ss;
        ss << "Autorisation accordée pour " << *avion;
        Logger::getInstance().log("APP", "AutorisationAtterrissage", ss.str());

        return true;
    }
    return false;
}

void APP::mettreAJour() {
    std::lock_guard<std::recursive_mutex> lock(mutexAPP_);

    // --- 1. PRIORITÉ ABSOLUE : GESTION DES URGENCES ---
    // On cherche s'il y a un avion en urgence qui attend (quelque soit sa place dans la file)
    for (Avion* avion : avionsDansZone_) {
        // Si l'avion est en urgence ET qu'il attend l'atterrissage
        if (avion->estEnUrgence() && avion->getEtat() == EtatAvion::EN_ATTENTE_ATTERRISSAGE) {

            // On demande l'autorisation prioritaire à la tour
            if (demanderAutorisationAtterrissage(avion)) {
                std::cout << "[APP] URGENCE - PRIORITE D'ATTERRISSAGE ACCORDEE a " << avion->getNom() << " - Il double la file d'attente.\n";
                // Note : L'avion reste techniquement dans la std::queue, mais son état change.
                // On gérera ce "fantôme" dans l'étape 2.
                return; // On a géré une urgence, on s'arrête là pour ce cycle pour lui laisser la place
            }
        }
    }

    // --- 2. GESTION DE LA FILE STANDARD ---
    if (!fileAttenteAtterrissage_.empty()) {
        Avion* avionEnAttente = fileAttenteAtterrissage_.front();

        // NETTOYAGE : Si l'avion en tête de file n'est plus en attente (ex: il a été géré en urgence ci-dessus)
        // on le retire simplement de la file et on passe au suivant.
        if (avionEnAttente->getEtat() != EtatAvion::EN_ATTENTE_ATTERRISSAGE) {
            fileAttenteAtterrissage_.pop();
            return;
        }

        // Si la TWR a une urgence en cours (décollage ou autre), on bloque la file standard
        if (twr_->estUrgenceEnCours()) return;

        // Tentative normale
        if (demanderAutorisationAtterrissage(avionEnAttente)) {
            fileAttenteAtterrissage_.pop();
            std::cout << "[APP] " << avionEnAttente->getNom() << " n'est plus pris en charge par l'APP. Atterrissage en cours.\n";
        }
    }

    // --- 3. DETECTION INITIALE DES URGENCES (CODE EXISTANT) ---
    for (Avion* avion : avionsDansZone_) {
        if (avion->estEnUrgence() && !twr_->estUrgenceEnCours()) {
            // Si c'est une nouvelle urgence qui n'est pas encore en approche ou atterrissage
            if (avion->getEtat() != EtatAvion::ATTERRISSAGE && avion->getEtat() != EtatAvion::EN_APPROCHE) {
                gererUrgence(avion);
            }
        }
    }
}

void APP::gererUrgence(Avion* avion) {
    twr_->setUrgenceEnCours(true);

    std::string typeTxt = "INCONNU";
    switch (avion->getTypeUrgence()) { // Suppose que tu as ajouté le getter dans Avion
    case TypeUrgence::PANNE_MOTEUR: typeTxt = "PANNE MOTEUR"; break;
    case TypeUrgence::MEDICAL: typeTxt = "MEDICAL"; break;
    case TypeUrgence::CARBURANT: typeTxt = "CARBURANT"; break;
    default: break;
    }
    std::cout << "[APP] URGENCE TYPE : " << typeTxt << " pour " << avion->getNom() << ". Priorite absolue.\n";

    Position posPiste = twr_->getPositionPiste();

    std::vector<Position> trajectoireDirecte;


    trajectoireDirecte.push_back(Position(posPiste.getX(), posPiste.getY(), 1000)); // point d'alignement avec la piste
    trajectoireDirecte.push_back(posPiste); // le point ou l'avion touche le sol

    avion->setTrajectoire(trajectoireDirecte);
    avion->setEtat(EtatAvion::EN_APPROCHE); // On force l'état approche

    std::cout << "[APP] Trajectoire DIRECTE d'urgence transmise a " << avion->getNom() << ".\n";
}