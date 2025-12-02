#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cmath>
#include <iostream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <fstream>

class Logger {
private:
    std::ofstream fichier_;
    std::mutex mutex_;
    bool premierElement_;

    Logger();
    ~Logger();

public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& getInstance();
    void log(const std::string& acteur, const std::string& action, const std::string& details);
};


class Avion;
class Parking;
class TWR;
class APP;
class CCR;
struct Aeroport;

class Position {
private:
    double x_, y_, altitude_;
public:
    Position(double x = 0, double y = 0, double z = 0);
    double getX() const;
    double getY() const;
    double getAltitude() const;
    void setPosition(double x, double y, double alt);
    double distance(const Position& other) const;
    friend std::ostream& operator<<(std::ostream& os, const Position& pos);
};

enum class EtatAvion {
    STATIONNE,
    EN_ATTENTE_DECOLLAGE,
    ROULE_VERS_PISTE,
    EN_ATTENTE_PISTE,
    DECOLLAGE,
    EN_ROUTE,
    EN_APPROCHE,
    EN_ATTENTE_ATTERRISSAGE,
    ATTERRISSAGE,
    ROULE_VERS_PARKING,
    TERMINE
};

enum class TypeUrgence {
    AUCUNE,
    CARBURANT,
    PANNE_MOTEUR,
    MEDICAL
};

class Parking {
private:
    std::string nom_;
    bool occupe_;
    Position pos_;
public:
    Parking(const std::string& nom, Position pos);
    const std::string& getNom() const;
    double getDistancePiste(const Position& posPiste) const;
    const Position& getPosition() const;
    bool estOccupe() const;
    void occuper();
    void liberer();
};

class Avion {
private:
    std::string nom_;
    float vitesse_;
    float vitesseSol_;
    float carburant_;
    float conso_;
    Position pos_;
    std::vector<Position> trajectoire_;
    EtatAvion etat_;
    float dureeStationnement_;
    Parking* parking_;
    Aeroport* destination_;
    TypeUrgence typeUrgence_;
    mutable std::mutex mtx_;

public:
    Avion(std::string n, float v, float vSol, float c, float conso, float dureeStat, Position pos);

    std::string getNom() const;
    float getVitesse() const;
    float getVitesseSol() const;
    float getCarburant() const;
    float getConsommation() const;
    Position getPosition() const;
    const std::vector<Position> getTrajectoire() const;
    EtatAvion getEtat() const;
    Parking* getParking() const;
    Aeroport* getDestination() const;
    float getDureeStationnement() const;
    bool estEnUrgence() const;
    TypeUrgence getTypeUrgence() const;

    void setPosition(const Position& p);
    void setTrajectoire(const std::vector<Position>& traj);
    void setEtat(EtatAvion e);
    void setParking(Parking* p);
    void setDestination(Aeroport* dest);

    void declarerUrgence(TypeUrgence type);
    void avancer(float dt);
    void avancerSol(float dt);
    void effectuerMaintenance();
    friend std::ostream& operator<<(std::ostream& os, const Avion& avion);
};

class TWR {
private:
    float tempsAtterrissageDecollage_;
    bool pisteLibre_;
    Position posPiste_;
    std::vector<Parking> parkings_;
    std::vector<Avion*> filePourDecollage_;
    bool urgenceEnCours_;
    mutable std::mutex mutexTWR_;

public:
    TWR(const std::vector<Parking>& parkings, Position posPiste, float tempsAtterrisageDecollage);

    Position getPositionPiste() const;
    bool estPisteLibre() const;
    void libererPiste();
    void reserverPiste();

    bool autoriserAtterrissage(Avion* avion);
    Parking* choisirParkingLibre();
    void attribuerParking(Avion* avion, Parking* parking);
    void gererRoulageVersParking(Avion* avion, Parking* p);

    void enregistrerPourDecollage(Avion* avion);
    bool autoriserDecollage(Avion* avion);
    Avion* choisirAvionPourDecollage() const;
    void retirerAvionDeDecollage(Avion* avion);

    void setUrgenceEnCours(bool statut);
    bool estUrgenceEnCours() const;
};

class APP {
private:
    std::vector<Avion*> avionsDansZone_;
    std::queue<Avion*> fileAttenteAtterrissage_;
    TWR* twr_;
    std::recursive_mutex mutexAPP_;

public:
    APP(TWR* tour);

    void ajouterAvion(Avion* avion);
    void assignerTrajectoireApproche(Avion* avion);
    void mettreEnAttente(Avion* avion);
    bool demanderAutorisationAtterrissage(Avion* avion);
    void mettreAJour();
    void gererUrgence(Avion* avion);
    size_t getNombreAvionsDansZone() const;
};

class CCR {
private:
    std::vector<Avion*> avionsEnCroisiere_;
    std::mutex mutexCCR_;

public:
    CCR();

    void prendreEnCharge(Avion* avion);
    void gererEspaceAerien();
    void transfererVersApproche(Avion* avion, APP* appCible);
};

struct Aeroport {
    std::string nom;
    Position position;
    float rayonControle;
    TWR* twr;
    APP* app;
    std::vector<Parking> parkings;

    Aeroport(std::string n, Position pos, float rayon = 100000.0f);
};
