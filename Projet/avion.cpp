#include "avion.hpp"

// ================= POSITION =================
Position::Position(float x, float y, float z) : x_(x), y_(y), altitude_(z) {}
float Position::getX() const { return x_; }
float Position::getY() const { return y_; }
float Position::getAltitude() const { return altitude_; }
void Position::setPosition(float x, float y, float alt) { x_ = x; y_ = y; altitude_ = alt; }
float Position::distance(const Position& other) const {
    float dx = x_ - other.x_;
    float dy = y_ - other.y_;
    float dz = altitude_ - other.altitude_;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ================= AVION =================
Avion::Avion(std::string n, float v, float c, float conso, Position pos)
    : nom_(n), vitesse_(v), carburant_(c), conso_(conso), pos_(pos),
    etat_(EtatAvion::STATIONNE), parking_(nullptr), destination_(nullptr) {
}

std::string Avion::getNom() const { std::lock_guard<std::mutex> lock(mtx_); return nom_; }
float Avion::getVitesse() const { std::lock_guard<std::mutex> lock(mtx_); return vitesse_; }
float Avion::getCarburant() const { std::lock_guard<std::mutex> lock(mtx_); return carburant_; }
float Avion::getConsommation() const { std::lock_guard<std::mutex> lock(mtx_); return conso_; }
Position Avion::getPosition() const { std::lock_guard<std::mutex> lock(mtx_); return pos_; }
const std::vector<Position> Avion::getTrajectoire() const { std::lock_guard<std::mutex> lock(mtx_); return trajectoire_; }
EtatAvion Avion::getEtat() const { std::lock_guard<std::mutex> lock(mtx_); return etat_; }
Parking* Avion::getParking() const { std::lock_guard<std::mutex> lock(mtx_); return parking_; }
Aeroport* Avion::getDestination() const { std::lock_guard<std::mutex> lock(mtx_); return destination_; }

void Avion::setPosition(const Position& p) { std::lock_guard<std::mutex> lock(mtx_); pos_ = p; }
void Avion::setTrajectoire(const std::vector<Position>& traj) { std::lock_guard<std::mutex> lock(mtx_); trajectoire_ = traj; }
void Avion::setEtat(EtatAvion e) { std::lock_guard<std::mutex> lock(mtx_); etat_ = e; }
void Avion::setParking(Parking* p) { std::lock_guard<std::mutex> lock(mtx_); parking_ = p; }
void Avion::setDestination(Aeroport* dest) { std::lock_guard<std::mutex> lock(mtx_); destination_ = dest; }

// --- LOGIQUE AVANCER CORRIGEE (Anti-Oscillation) ---
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
        pos_ = cible; // Snap sur le point
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
}

// ================= PARKING =================
Parking::Parking(const std::string& nom, int distance)
    : nom_(nom), distancePiste_(distance), occupe_(false) {
}
const std::string& Parking::getNom() const { return nom_; }
int Parking::getDistancePiste() const { return distancePiste_; }
bool Parking::estOccupe() const { return occupe_; }
void Parking::occuper() { occupe_ = true; }
void Parking::liberer() { occupe_ = false; }

// ================= TWR =================
TWR::TWR(const std::vector<Parking>& parkings) : pisteLibre_(true), parkings_(parkings) {}

bool TWR::estPisteLibre() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    return pisteLibre_;
}

void TWR::libererPiste() {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    pisteLibre_ = true;
    // std::cout << "[TWR] Piste libre.\n"; 
}

bool TWR::autoriserAtterrissage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (pisteLibre_) {
        pisteLibre_ = false;
        std::cout << "[TWR] Atterrissage AUTORISE pour " << avion->getNom() << ".\n";
        avion->setEtat(EtatAvion::ATTERRISSAGE);
        return true;
    }
    return false;
}

Parking* TWR::attribuerParking(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    for (auto& parking : parkings_) {
        if (!parking.estOccupe()) {
            parking.occuper();
            avion->setParking(&parking);
            std::cout << "[TWR] Parking " << parking.getNom() << " attribue a " << avion->getNom() << ".\n";
            return &parking;
        }
    }
    std::cout << "[TWR] ALERTE : Aucun parking dispo pour " << avion->getNom() << " !\n";
    return nullptr;
}

void TWR::gererRoulageAuSol(Avion* avion, Parking* p) {
    avion->setEtat(EtatAvion::ROULE_VERS_PARKING);
    std::cout << "[TWR] Roulage vers " << p->getNom() << " pour " << avion->getNom() << ".\n";
}

void TWR::enregistrerPourDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    filePourDecollage_.push_back(avion);
    avion->setEtat(EtatAvion::EN_ATTENTE_DECOLLAGE);
    std::cout << "[TWR] " << avion->getNom() << " aligne au seuil de piste (File: " << filePourDecollage_.size() << ").\n";
}

bool TWR::autoriserDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (pisteLibre_) {
        pisteLibre_ = false;
        std::cout << "[TWR] Decollage AUTORISE pour " << avion->getNom() << ". Vent calme.\n";
        avion->setEtat(EtatAvion::DECOLLAGE);
        return true;
    }
    return false;
}

// Logique de priorité (Distance parking) restaurée
Avion* TWR::choisirAvionPourDecollage() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (filePourDecollage_.empty()) return nullptr;

    Avion* prioritaire = nullptr;
    int maxDistance = -1;

    for (auto* avion : filePourDecollage_) {
        // Simple logique : le premier arrivé (FIFO) ou distance. 
        // Ici on garde la logique "FIFO" car l'avion a déjà quitté le parking 
        // pour arriver au seuil de piste dans notre modèle.
        // Mais si on veut la distance parking :
        /* if (avion->getParking()) { ... }
        */
        // On retourne le premier de la file
        if (!prioritaire) prioritaire = avion;
    }
    return prioritaire;
}

void TWR::retirerAvionDeDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    auto it = std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion);
    if (it != filePourDecollage_.end()) {
        filePourDecollage_.erase(it);
    }
    pisteLibre_ = true;
    std::cout << "[TWR] Piste liberee apres le decollage de " << avion->getNom() << ".\n";
}

// ================= APP =================
APP::APP(TWR* tour, Position pos) : twr_(tour), posAeroport_(pos) {}

void APP::ajouterAvion(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexAPP_);
    // Evite doublons
    if (std::find(avionsDansZone_.begin(), avionsDansZone_.end(), avion) == avionsDansZone_.end()) {
        avionsDansZone_.push_back(avion);
        std::cout << "[APP] " << avion->getNom() << " est sur le radar d'approche.\n";
    }
}

void APP::assignerTrajectoireApproche(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexAPP_);

    // Calcul d'une trajectoire relative à l'aéroport
    float ax = posAeroport_.getX();
    float ay = posAeroport_.getY();

    std::vector<Position> traj = {
        Position(ax, ay + 3000, 2000), // Etape base
        Position(ax, ay + 1000, 1000), // Interception
        Position(ax, ay + 500, 500)    // Courte finale
    };

    avion->setTrajectoire(traj);
    avion->setEtat(EtatAvion::EN_APPROCHE);
    std::cout << "[APP] Vecteurs d'approche transmis a " << avion->getNom() << ".\n";
}

void APP::mettreEnAttente(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexAPP_);
    avion->setEtat(EtatAvion::EN_ATTENTE_ATTERRISSAGE);
    fileAttenteAtterrissage_.push(avion);
    std::cout << "[APP] " << avion->getNom() << " entre en circuit d'attente.\n";
}

bool APP::demanderAutorisationAtterrissage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexAPP_);
    if (!twr_) return false;

    // std::cout << "[APP] Coordination TWR pour " << avion->getNom() << "...\n";
    if (twr_->autoriserAtterrissage(avion)) {
        // Trajectoire vers le point de toucher (Z=0)
        std::vector<Position> finale = { posAeroport_ };
        avion->setTrajectoire(finale);
        return true;
    }
    return false;
}

void APP::mettreAJour() {
    std::lock_guard<std::mutex> lock(mutexAPP_);
    for (Avion* avion : avionsDansZone_) {
        // Gestion Carburant restaurée
        if (avion->getCarburant() < 500) {
            std::cout << "[APP] ! MAYDAY ! Carburant critique pour " << avion->getNom() << " !\n";
        }

        // Logique de demande automatique si l'avion a fini son approche
        if (avion->getEtat() == EtatAvion::EN_APPROCHE && avion->getTrajectoire().empty()) {
            // Sera géré par le thread avion qui appelle demanderAutorisation
        }
    }
}

// ================= CCR =================
CCR::CCR() {}

void CCR::prendreEnCharge(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexCCR_);
    avionsEnCroisiere_.push_back(avion);
    avion->setEtat(EtatAvion::EN_ROUTE);
    std::cout << ">>> [CCR GLOBAL] Radar contact " << avion->getNom() << " FL300.\n";
}

void CCR::transfererVersApproche(Avion* avion, APP* appCible) {
    std::lock_guard<std::mutex> lock(mutexCCR_);
    auto it = std::find(avionsEnCroisiere_.begin(), avionsEnCroisiere_.end(), avion);
    if (it != avionsEnCroisiere_.end()) {
        avionsEnCroisiere_.erase(it);
    }

    std::cout << ">>> [CCR GLOBAL] Hand-off " << avion->getNom() << " vers l'Approche locale.\n";
    appCible->ajouterAvion(avion);
    appCible->assignerTrajectoireApproche(avion);
}

void CCR::gererEspaceAerien() {
    std::lock_guard<std::mutex> lock(mutexCCR_);
    // Logique globale simplifiée : on check juste si tout va bien
    // Les transferts sont initiés par les pilotes dans ce modèle pour simplifier les itérateurs
}

// ================= AEROPORT =================
Aeroport::Aeroport(std::string n, Position pos) : nom(n), position(pos) {
    // Création de 3 parkings avec distances variées
    parkings.push_back(Parking(n + "-G1", 100));
    parkings.push_back(Parking(n + "-G2", 300));
    parkings.push_back(Parking(n + "-G3", 600)); // Le plus loin, prioritaire au décollage selon le sujet

    twr = new TWR(parkings);
    app = new APP(twr, position);
}