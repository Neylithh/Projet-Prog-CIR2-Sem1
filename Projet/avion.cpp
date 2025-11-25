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

// Les getters et setters de Avion utilisent std::lock_guard pour assurer la thread-safety de l'objet Avion.
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

void Avion::avancer(float dt) {
    std::lock_guard<std::mutex> lock(mtx_);
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

void Avion::setParking(Parking* p) {
    std::lock_guard<std::mutex> lock(mtx_);
    parking_ = p;
}

Parking* Avion::getParking() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return parking_;
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

// Ajout du lock pour garantir une lecture thread-safe de pisteLibre_
bool TWR::estPisteLibre() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
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
            avion->setParking(&parking);
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
        << ". Distance à la piste : " << distance << " units." << std::endl;
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

// Ajout du lock pour garantir un accès thread-safe à filePourDecollage_
Avion* TWR::choisirAvionPourDecollage() const {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    if (filePourDecollage_.empty()) {
        return nullptr;
    }
    Avion* prioritaire = nullptr;
    int maxDistance = -1;

    for (auto* avion : filePourDecollage_) {
        Parking* p = avion->getParking();
        if (p) {
            int dist = p->getDistancePiste();
            if (dist > maxDistance) {
                maxDistance = dist;
                prioritaire = avion;
            }
        }
    }

    if (!prioritaire && !filePourDecollage_.empty()) {
        // En cas d'avion sans parking (ce qui ne devrait pas arriver ici si la logique est suivie), prendre le premier
        prioritaire = filePourDecollage_.front();
    }
    return prioritaire;
}

// Correction du deadlock: On ne peut pas appeler libererPiste() ici car le mutexTWR_ est déjà verrouillé.
void TWR::retirerAvionDeDecollage(Avion* avion) {
    std::lock_guard<std::mutex> lock(mutexTWR_);
    auto it = std::find(filePourDecollage_.begin(), filePourDecollage_.end(), avion);
    if (it != filePourDecollage_.end()) {
        filePourDecollage_.erase(it);
        std::cout << "[TWR] " << avion->getNom() << " retiré de la file de décollage." << std::endl;
    }
    // Libération manuelle de la piste
    pisteLibre_ = true;
    std::cout << "[TWR] La piste a été libérée par " << avion->getNom() << " (après décollage)." << std::endl;
}

APP::APP(TWR* tour)
    : twr(tour), altitudeAttente(3000), rayonAttente(5000)//appel au constructeur pour initialiser les valeurs de rayon attente et altitude attente 
{
}

void APP::ajouterAvion(Avion* avion)//ajoute un avion à la zone d'approche 
{
    std::lock_guard<std::mutex> lock(mtx);

    avionsDansZone.push_back(avion);
    avion->setEtat(EtatAvion::EN_APPROCHE);

    std::cout << "[APP] Avion " << avion->getNom()
        << " entre dans la zone d’approche.\n";
}

void APP::assignerTrajectoire(Avion* avion)//donne la trajectoire pour acceder a la piste d'aterissage 
{
    std::lock_guard<std::mutex> lock(mtx);

    std::vector<Position> traj = {//trajectoire pour attérir 
        Position(0, 0, 2000),
        Position(0, -3000, 1000),
        Position(0, -5000, 0)
    };

    avion->setTrajectoire(traj);
    avion->setEtat(EtatAvion::EN_APPROCHE);

    std::cout << "[APP] Trajectoire d’approche assignée à "
        << avion->getNom() << ".\n";
}

void APP::mettreEnAttente(Avion* avion)//ajoute un avion a la file d'attente 
{
    std::lock_guard<std::mutex> lock(mtx);

    avion->setEtat(EtatAvion::EN_ATTENTE);

    std::vector<Position> holding = {//circuit d'attente 
        Position(rayonAttente, 0, altitudeAttente),
        Position(0, rayonAttente, altitudeAttente),
        Position(-rayonAttente, 0, altitudeAttente),
        Position(0, -rayonAttente, altitudeAttente)
    };

    avion->setTrajectoire(holding);

    fileAttenteAtterrissage.push(avion);

    std::cout << "[APP] Avion " << avion->getNom()
        << " placé dans la file d’attente.\n";
}

Avion* APP::prochainPourAtterrissage()//prend le prochain de la file 
{
    std::lock_guard<std::mutex> lock(mtx);

    if (fileAttenteAtterrissage.empty())
        return nullptr;

    Avion* a = fileAttenteAtterrissage.front();
    fileAttenteAtterrissage.pop();

    return a;
}

bool APP::demanderAutorisationAtterrissage(Avion* avion)//demande a twr (tour de controle)si c'est possible d'attérir 
{
    std::lock_guard<std::mutex> lock(mtx);

    if (!twr)
        return false;

    std::cout << "[APP] Demande d’autorisation d’atterrissage pour "
        << avion->getNom() << "…\n";
    //dans le cas ou la tour accepte que l'avion attérisse 
    if (twr->autoriserAtterrissage(avion)) {

        std::cout << "[APP] Autorisation accordée.\n";

        avion->setEtat(EtatAvion::ATTERRISSAGE);

        std::vector<Position> finale = {//trajectoire finale pour attérir 
            Position(0, -1000, 500),
            Position(0, -2000, 0)
        };

        avion->setTrajectoire(finale);
        return true;
    }
    //dans le cas ou la demande d'aterissage est refusée 
    std::cout << "[APP] Autorisation refusée : avion remis en attente.\n";
    // Mettre l'avion en attente si l'APP le juge nécessaire, mais ici on va laisser TWR gérer l'état
    // pour simplifier le flux de contrôle. S'il est refusé, il reste EN_APPROCHE en attente de la prochaine demande.
    // L'implémentation originale le remettait en attente : je la conserve pour la conformité à votre code.
    mettreEnAttente(avion);
    return false;
}

void APP::mettreAJour()//mise a jout régulière des avions dans la zone d'approche 
{
    std::lock_guard<std::mutex> lock(mtx);

    for (Avion* avion : avionsDansZone) {
        //met a jour la position de l'avion 
        avion->avancer(1.0f);
        //dans le cas ou l'avion a le carburant faible 
        if (avion->getCarburant() < 300) {
            std::cout << "[APP] CARBURANT FAIBLE : "
                << avion->getNom() << "\n";
        }//si l'avion n'as plus de trajectoire , il demande l'autorisation d'attérir 
        if (avion->getTrajectoire().empty() &&
            avion->getEtat() == EtatAvion::EN_APPROCHE)
        {
            // L'APP gère la demande d'autorisation
            demanderAutorisationAtterrissage(avion);
        }
    }
}