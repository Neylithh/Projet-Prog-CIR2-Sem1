#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cmath>
#include <iostream>
#include <queue>
#include <algorithm>

// --- FORWARD DECLARATIONS (Indispensable pour que tout tienne dans un fichier) ---
class Avion;
class Parking;
class TWR;
class APP;
class CCR;
struct Aeroport;

// --- POSITION ---
class Position {
private:
    float x_, y_, altitude_;
public:
    Position(float x = 0, float y = 0, float z = 0);
    float getX() const;
    float getY() const;
    float getAltitude() const;
    void setPosition(float x, float y, float alt);
    float distance(const Position& other) const;
};

// --- ETATS AVION ---
enum class EtatAvion {
    STATIONNE,              // Moteurs coupés
    ROULE_VERS_PISTE,       // Taxi
    EN_ATTENTE_DECOLLAGE,   // Au seuil de piste
    DECOLLAGE,              // Sur la piste -> Montée
    EN_ROUTE,               // Croisière (Géré par CCR)
    EN_APPROCHE,            // Descente (Géré par APP)
    EN_ATTENTE_ATTERRISSAGE,// Circuit d'attente
    ATTERRISSAGE,           // Finale
    ROULE_VERS_PARKING,     // Dégagement piste
    TERMINE                 // Vol fini
};

// --- PARKING ---
class Parking {
private:
    std::string nom_;
    int distancePiste_;
    bool occupe_;
public:
    Parking(const std::string& nom, int distance);
    const std::string& getNom() const;
    int getDistancePiste() const;
    bool estOccupe() const;
    void occuper();
    void liberer();
};

// --- AVION ---
class Avion {
private:
    std::string nom_;
    float vitesse_;
    float carburant_;
    float conso_;
    Position pos_;
    std::vector<Position> trajectoire_;
    EtatAvion etat_;

    Parking* parking_;       // Parking actuel ou assigné
    Aeroport* destination_;  // Aéroport de destination

    mutable std::mutex mtx_;

public:
    Avion(std::string n, float v, float c, float conso, Position pos);

    // Getters Thread-Safe
    std::string getNom() const;
    float getVitesse() const;
    float getCarburant() const;
    float getConsommation() const;
    Position getPosition() const;
    const std::vector<Position> getTrajectoire() const;
    EtatAvion getEtat() const;
    Parking* getParking() const;
    Aeroport* getDestination() const;

    // Setters Thread-Safe
    void setPosition(const Position& p);
    void setTrajectoire(const std::vector<Position>& traj);
    void setEtat(EtatAvion e);
    void setParking(Parking* p);
    void setDestination(Aeroport* dest);

    // Mouvement (Version corrigée anti-oscillation)
    void avancer(float dt);
};

// --- TWR (TOUR DE CONTROLE) ---
class TWR {
private:
    bool pisteLibre_;
    std::vector<Parking> parkings_;
    std::vector<Avion*> filePourDecollage_;
    mutable std::mutex mutexTWR_;
public:
    TWR(const std::vector<Parking>& parkings);

    bool estPisteLibre() const;
    void libererPiste();

    // Atterrissage
    bool autoriserAtterrissage(Avion* avion);
    Parking* attribuerParking(Avion* avion); // Logique priorité distance restaurée
    void gererRoulageAuSol(Avion* avion, Parking* p);

    // Décollage
    void enregistrerPourDecollage(Avion* avion);
    bool autoriserDecollage(Avion* avion);
    Avion* choisirAvionPourDecollage() const; // Logique priorité restaurée
    void retirerAvionDeDecollage(Avion* avion);
};

// --- APP (APPROCHE) ---
class APP {
private:
    std::vector<Avion*> avionsDansZone_;
    std::queue<Avion*> fileAttenteAtterrissage_;
    TWR* twr_;
    Position posAeroport_; // Pour calculer les trajectoires relatives
    std::mutex mutexAPP_;

    double altitudeAttente_ = 3000;
    double rayonAttente_ = 5000;

public:
    APP(TWR* tour, Position posAero);
    void ajouterAvion(Avion* avion);
    void assignerTrajectoireApproche(Avion* avion);
    void mettreEnAttente(Avion* avion);
    bool demanderAutorisationAtterrissage(Avion* avion);
    void mettreAJour(); // Gestion carburant et urgences restaurée
};

// --- CCR (CENTRE DE CONTROLE REGIONAL) ---
class CCR {
private:
    std::vector<Avion*> avionsEnCroisiere_;
    std::mutex mutexCCR_;
public:
    CCR();
    void prendreEnCharge(Avion* avion);
    void gererEspaceAerien(); // Scan global
    void transfererVersApproche(Avion* avion, APP* appCible);
};

// --- AEROPORT ---
struct Aeroport {
    std::string nom;
    Position position;
    TWR* twr;
    APP* app;
    std::vector<Parking> parkings;

    // Le constructeur sera dans le .cpp ou inline ici si simple
    Aeroport(std::string n, Position pos);
};