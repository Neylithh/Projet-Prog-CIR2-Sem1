#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#pragma once

class Avion {
private:
    std::string nom_;
    float vitesse_;
    float carburant_;
    float conso_;
    Position pos_;
    std::vector<Position> trajectoire_;  // liste de points à suivre
    EtatAvion etat_;

public:
    Avion(std::string n, float v, float c, float conso,
        Position pos, const std::vector<Position>& traj);
    std::string getNom() const;
    float getVitesse() const;
    float getCarburant() const;
    float getConsommation() const;
    Position getPosition() const;
    const std::vector<Position>& getTrajectoire() const;
    EtatAvion getEtat() const;
    void setPosition(const Position& p);
    void setTrajectoire(const std::vector<Position>& traj);
    void setEtat(EtatAvion e);
    void avancer(float temps = 1.0f);
};

class Position {
private:
    float x_;
    float y_;
    float altitude_;

public:
    Position(float x = 0, float y = 0, float z = 0)
        : x_(x), y_(y), altitude_(z) {
    }
    float getX() const { return x_; }
    float getY() const { return y_; }
    float getAltitude() const { return altitude_; }
    void setPosition(float x, float y, float alt) {
        x_ = x;
        y_ = y;
        altitude_ = alt;
    }
};

enum class EtatAvion {
    DECOLLAGE,
    EN_ROUTE,
    EN_APPROCHE,
    EN_ATTENTE,
    ATTERRISSAGE,
    ROULE,
    STATIONNE
};

class Parking {
private:
    std::string nom;
    int distancePiste;    
    bool occupe;          

public:
    Parking(const std::string& nom, int distance);
    const std::string& getNom() const;
    int getDistancePiste() const;
    bool estOccupe() const;
    void occuper();
    void liberer();
};

class TWR {
private :
    bool pisteLibre;
    std::vector<Parking> parkings;
};