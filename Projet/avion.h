#include <string>
#include <iostream>
#pragma once

class Avion {
private :
	std::string nom_;
	float vitesse_;
	float carburant_;
	float conso_;
	Position pos_;
	Position arrivee_; // à modifier je ne pense pas l'avoir bien défini
	EtatAvion etat_;
public:
	Avion(std::string n, float v, float c, float conso, Position pos, Position traj)
		: nom_(n), vitesse_(v), carburant_(c), conso_(conso), pos_(pos), trajectoire_(traj) {}
	std::string getNom() const { return nom_; }
	float getVitesse() const { return vitesse_; }
	float getCarburant() const { return carburant_; }
	float getConsommation() const { return conso_; }
	Position getPosition() const { return pos_; }
	Position getTrajectoire() const { return trajectoire_; }
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

class EtatsAvion {


class Controleur {
	// probablement classe abstraite (vu quil ya 3 controlleur différents : Centres de Contrôle Régional (CCR/ACC), Centres de Contrôle d'Approche (APP) et Tours de Contrôle d'Aérodrome (TWR)