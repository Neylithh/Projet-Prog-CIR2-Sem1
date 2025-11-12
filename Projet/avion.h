#include <string>
#include <iostream>

class Avion {
private :
	std::string nom_;
	float vitesse_;
	float carburant_;
	float conso_;
	Position pos_;
public:
	Avion(std::string n, float v, float c, float conso, Position pos)
		: nom_(n), vitesse_(v), carburant_(c), conso_(conso), pos_(pos) {}
	std::string getNom() const { return nom_; }
	float getVitesse() const { return vitesse_; }
	float getCarburant() const { return carburant_; }
	float getConsommation() const { return conso_; }
	Position getPosition() const { return pos_; }
};

class Position {
private:
	float x_;
	float y_;
	float z_;
public:
	Position(float x = 0, float y = 0, float z = 0) : x_(x), y_(y), z_(z) {}
	float getX() const { return x_; }
	float getY() const { return y_; }
	float getZ() const { return z_; }
}

class Controleur {
