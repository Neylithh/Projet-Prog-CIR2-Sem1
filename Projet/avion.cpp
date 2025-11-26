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
    : nom_(n), vitesse_(v), carburant_(c), conso_(conso), pos_(pos), trajectoire_(traj), etat_(EtatAvion::EN_ROUTE), parking_(nullptr) {
}

std::string Avion::getNom() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return nom_;
}
float Avion::getVitesse() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return vitesse_;
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
const std::vector<Position> Avion::getTrajectoire() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return trajectoire_;
}
EtatAvion Avion::getEtat() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return etat_;
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

// === CORRECTION MAJEURE ICI ===
void Avion::avancer(float dt) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (trajectoire_.empty())
        return;

    Position cible = trajectoire_.front();

    float dx = cible.getX() - pos_.getX();
    float dy = cible.getY() - pos_.getY();
    float dz = cible.getAltitude() - pos_.getAltitude();
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Calcul de la distance à parcourir sur ce tick
    float distance_a_parcourir = vitesse_ * dt;

    // Si on est assez proche pour atteindre le point en ce tick (ou si on est dessus)
    // On se "colle" au point pour éviter l'oscillation (overshoot)
    if (dist <= distance_a_parcourir) {
        pos_ = cible; // On se place exactement sur le point
        trajectoire_.erase(trajectoire_.begin()); // On retire le point atteint
    }
    else {
        // Sinon, on avance vers le point (vecteur normalisé * distance)
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

void Avion::setParking(Parking* p) {
    std::lock_guard<std::mutex> lock(mtx_);
    parking_ = p;
}

Parking* Avion::getParking() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return parking_;
}


Parking::Parking(const std::string& nom, int distance)
    : nom_(nom), distancePiste_(distance), occupe_(false) {
}

const std::string& Parking::getNom() const {
    return nom_;
}

int Parking::getDistancePiste() const {
    return distancePiste_;
}

bool Parking::estOccupe() const {
    return occupe_;
}

void Parking::occuper() {
    occupe_ = true;
}

void Parking::liberer() {
    occupe_ = false;
}

TWR::TWR(const std::vector<Parking>& parkings)
    : pisteLibre_(true), parkings_(parkings) {
}

bool TWR::estPisteLibre() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    return pisteLibre_;
}

bool TWR::autoriserAtterrissage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (pisteLibre_) {
        pisteLibre_ = false;
        std::cout << "[TWR] Atterrissage AUTORISE pour " << avion->getNom() << std::endl;
        avion->setEtat(EtatAvion::ATTERRISSAGE);
        return true;
    }
    // Message optionnel pour ne pas spammer la console si refusé
    // std::cout << "[TWR] Atterrissage REFUSE pour " << avion->getNom() << " (Piste occupée)" << std::endl;
    return false;
}

void TWR::libererPiste() {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    pisteLibre_ = true;
    std::cout << "[TWR] La piste a ete LIBEREE." << std::endl;
}

Parking* TWR::attribuerParking(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    for (auto& parking : parkings_) {
        if (!parking.estOccupe()) {
            parking.occuper();
            avion->setParking(&parking);
            std::cout << "[TWR] Parking " << parking.getNom()
                << " attribue a " << avion->getNom() << std::endl;
            return &parking;
        }
    }
    std::cout << "[TWR] Aucun parking disponible pour " << avion->getNom() << std::endl;
    return nullptr;
}

void TWR::gererRoulage(Avion* avion, Parking* parking) {
    avion->setEtat(EtatAvion::ROULE);
    int distance = parking->getDistancePiste();
    std::cout << "[TWR] ROULAGE vers parking pour " << avion->getNom() << std::endl;
}

void TWR::enregistrerPourDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    filePourDecollage_.push_back(avion);
    avion->setEtat(EtatAvion::EN_ATTENTE);
    std::cout << "[TWR] " << avion->getNom() << " ajoute a la file de decollage.\n";
}

bool TWR::autoriserDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (pisteLibre_) {
        pisteLibre_ = false;
        std::cout << "[TWR] Decollage AUTORISE pour " << avion->getNom() << "." << std::endl;
        avion->setEtat(EtatAvion::DECOLLAGE);
        return true;
    }
    return false;
}

Avion* TWR::choisirAvionPourDecollage() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (filePourDecollage_.empty()) {
        return nullptr;
    }
    Avion* prioritaire = nullptr;
    int maxDistance = -1;

    for (auto* avion : filePourDecollage_) {
        Parking* p = avion->getParking();
        // Attention: Si l'avion quitte le parking, getParking peut être null.
        // Ici on suppose qu'on a gardé l'info ou que l'avion "sort" du parking.
        // Simplification : on prend le premier s'il n'a plus de parking assigné.
        if (p) {
            int dist = p->getDistancePiste();
            if (dist > maxDistance) {
                maxDistance = dist;
                prioritaire = avion;
            }
        }
        else {
            // Si pas de parking (ex: déjà en roulage vers piste), on le considère valide
            if (maxDistance == -1) prioritaire = avion;
        }
    }

    if (!prioritaire && !filePourDecollage_.empty()) {
        prioritaire = filePourDecollage_.front();
    }
    return prioritaire;
}

void TWR::retirerAvionDeDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    auto it = std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion);
    if (it != filePourDecollage_.end()) {
        filePourDecollage_.erase(it);
        std::cout << "[TWR] " << avion->getNom() << " retire de la file.\n";
    }
    pisteLibre_ = true;
    std::cout << "[TWR] Piste LIBEREE apres decollage de " << avion->getNom() << ".\n";
}

APP::APP(TWR* tour)
    : twr_(tour), altitudeAttente_(3000), rayonAttente_(5000)
{
}

void APP::ajouterAvion(Avion* avion)
{
    std::lock_guard<std::mutex> lock(mutexAPP_);
    // Vérif simple pour éviter les doublons si appelé plusieurs fois
    if (std::find(avionsDansZone_.begin(), avionsDansZone_.end(), avion) == avionsDansZone_.end()) {
        avionsDansZone_.push_back(avion);
        avion->setEtat(EtatAvion::EN_APPROCHE);
        std::cout << "[APP] " << avion->getNom() << " contacte l'approche.\n";
    }
}

void APP::assignerTrajectoire(Avion* avion)
{
    std::lock_guard<std::mutex> lock(mutexAPP_);

    std::vector<Position> traj = {
        Position(0, 0, 2000),
        Position(0, -3000, 1000),
        Position(0, -5000, 0) // Point d'entrée de piste (fictif)
    };

    avion->setTrajectoire(traj);
    // On force l'état pour être sûr
    avion->setEtat(EtatAvion::EN_APPROCHE);

    std::cout << "[APP] Vecteurs d'approche envoyes a " << avion->getNom() << ".\n";
}

void APP::mettreEnAttente(Avion* avion)
{
    std::lock_guard<std::mutex> lock(mutexAPP_);

    avion->setEtat(EtatAvion::EN_ATTENTE);

    std::vector<Position> holding = {
        Position(rayonAttente_, 0, altitudeAttente_),
        Position(0, rayonAttente_, altitudeAttente_),
        Position(-rayonAttente_, 0, altitudeAttente_),
        Position(0, -rayonAttente_, altitudeAttente_)
    };
    // Boucler le holding ? Pour l'instant on laisse comme ça.
    avion->setTrajectoire(holding);

    fileAttenteAtterrissage_.push(avion);
    std::cout << "[APP] " << avion->getNom() << " entre en circuit d'attente.\n";
}

Avion* APP::prochainPourAtterrissage()
{
    std::lock_guard<std::mutex> lock(mutexAPP_);
    if (fileAttenteAtterrissage_.empty()) return nullptr;
    Avion* a = fileAttenteAtterrissage_.front();
    fileAttenteAtterrissage_.pop();
    return a;
}

bool APP::demanderAutorisationAtterrissage(Avion* avion)
{
    std::lock_guard<std::mutex> lock(mutexAPP_);

    if (!twr_) return false;

    // std::cout << "[APP] Demande atterrissage pour " << avion->getNom() << "...\n";

    if (twr_->autoriserAtterrissage(avion)) {
        std::cout << "[APP] Autorisation RECUE pour " << avion->getNom() << ". Debut finale.\n";

        // Trajectoire finale (Glideslope)
        std::vector<Position> finale = {
            Position(0, 0, 0) // Le point de toucher des roues
        };
        avion->setTrajectoire(finale);
        return true;
    }

    // Si refusé, et que l'avion n'est pas déjà en attente (et a fini sa traj d'approche)
    // On pourrait le mettre en attente.
    // Ici on ne fait rien, routine_avion réessayera ou APP le gérera.
    return false;
}

void APP::mettreAJour()
{
    std::lock_guard<std::mutex> lock(mutexAPP_);

    // On utilise une boucle par index ou itérateur, attention si on supprime des éléments
    for (Avion* avion : avionsDansZone_) {
        // En théorie, l'avion "avance" via son propre thread. 
        // L'APP ici surveille juste l'état, elle ne doit PAS appeler avion->avancer() 
        // sinon l'avion avance 2x plus vite (une fois dans son thread, une fois ici).
        // J'ai COMMENTE l'appel à avancer ici pour laisser le thread avion gérer la physique.
        // avion->avancer(1.0f); 

        if (avion->getCarburant() < 300) {
            std::cout << "[APP] ! MAYDAY CARBURANT ! : " << avion->getNom() << "\n";
        }
    }
}