#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#pragma once

class Avion {
private:
    std::string nom_; // nom de l'avion
    float vitesse_; // vitesse de l'avion
    float carburant_; // niveau de carburant
    float conso_; // consommation de carburant 
    Position pos_; // position de l'avion
    std::vector<Position> trajectoire_;  // liste de points à suivre
    EtatAvion etat_; // état de l'avion

public:
    Avion(std::string n, float v, float c, float conso, Position pos, const std::vector<Position>& traj); // constructeur de l'avion
    std::string getNom() const; // renvoie le nom de l'avion
    float getVitesse() const; // renvoie la vitesse de l'avion
    float getCarburant() const; // renvoie le carburant de l'avion
    float getConsommation() const; // renvoie la consommaiton de l'avion
    Position getPosition() const; // renvoie la position de l'avion
    const std::vector<Position>& getTrajectoire() const; // renvoie la trajectoire de l'avion
    EtatAvion getEtat() const; // renvoie l'état de l'avion
    void setPosition(const Position& p); // met à jour la position de l'avion
    void setTrajectoire(const std::vector<Position>& traj); // met à jour la trajectoire de l'avion
    void setEtat(EtatAvion e); // met à jour l'état de l'avion
    void avancer(float temps = 1.0f); // fais avancer l'avion vers le prochain point dans la trajectoire (avance un petit peu donc ne garantit pas qu'on atteigne le point en question)
};

class Position {
private:
    float x_; // position x
    float y_; // position y
    float altitude_; // altitude

public:
    Position(float x = 0, float y = 0, float z = 0); // constructeur
    float getX() const; // renvoie la coordonnée x
    float getY() const; // renvoie la coordonnée y
    float getAltitude() const; // renvoie l'altitude 
    void setPosition(float x, float y, float alt); // met la position aux coordonnées en paramètres
};

enum class EtatAvion { // liste des différents états pour un avion
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
    std::string nom; // nom du parking
    int distancePiste; // distance entre le parking et la piste utile pour la priorité pour la file de décollage
    bool occupe; // indique si le parking est occupé ou pas

public:
    Parking(const std::string& nom, int distance); // constructeur
    const std::string& getNom() const; // renvoie le nom du parking
    int getDistancePiste() const; // renvoie la distance entre le parking et la piste
    bool estOccupe() const; // renvoie si le parking est occupé ou non
    void occuper(); // change l'état du parking en occupé
    void liberer(); // change l'état du parking en libre
};

class TWR {
private :
    bool pisteLibre; // indique si la piste est libre ou pas
    std::vector<Parking> parkings; // liste des parkings
    std::vector<Avion*> filePourDecollage; // file des avions qui attendent de décoller selon la priorité : "Arbitrairement, l'avion le plus éloigné de la piste a la priorité au décollage"
public:
    bool autoriserAtterrissage(Avion* avion); // vérifie si la piste est libre si oui on la réserve pour l'avion qui atterit sinon on refuse
    void libererPiste(); // libère la piste après un atterrissage ou un décollage
    Parking* attribuerParking(Avion* avion); // assigne l'avion à un parking libre
    void gererRoulage(Avion* avion, Parking* parking); // gère toute la partie au sol de l'avion avant décollage ou après un atterrissage 
    void enregistrerPourDecollage(Avion* avion); // ajoute un avion dans la file de décollage quand il est prêt
    bool autoriserDecollage(Avion* avion); // vérifie si la piste est libre si oui réserver la piste pour l'avion qui décolle sinon on refuse
    Avion* choisirAvionPourDecollage() const; // décide qui va décoller en premier selon la priorité
};
