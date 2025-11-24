#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <queue>
#include <mutex>
#pragma once

class Position {
private:
    float x_; // position x
    float y_; // position y
    float altitude_; // altitude

public:
    // Arguments par défaut uniquement dans la déclaration (correct)
    Position(float x = 0, float y = 0, float z = 0); // constructeur
    float getX() const; // renvoie la coordonnée x
    float getY() const; // renvoie la coordonnée y
    float getAltitude() const; // renvoie l'altitude 
    void setPosition(float x, float y, float alt); // met la position aux coordonnes en paramètres
    float distance(const Position& other) const; // calcule la distance entre deux positions
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

class Parking;

class Avion {
private:
    std::string nom_; // nom de l'avion
    float vitesse_; // vitesse de l'avion
    float carburant_; // niveau de carburant
    float conso_; // consommation de carburant 
    Position pos_; // position de l'avion
    std::vector<Position> trajectoire_;  // liste de points à suivre
    EtatAvion etat_; // état de l'avion
    Parking* parking_;
    mutable std::mutex mtx_;

public:
    Avion(std::string n, float v, float c, float conso, Position pos, const std::vector<Position>& traj); // constructeur de l'avion
    std::string getNom() const; // renvoie le nom de l'avion
    float getVitesse() const; // renvoie la vitesse de l'avion
    float getCarburant() const; // renvoie le carburant de l'avion
    float getConsommation() const; // renvoie la consommaiton de l'avion
    Position getPosition() const; // renvoie la position de l'avion
    const std::vector<Position> getTrajectoire() const; // renvoie la trajectoire de l'avion
    EtatAvion getEtat() const; // renvoie l'état de l'avion
    void setPosition(const Position& p); // met à jour la position de l'avion
    void setTrajectoire(const std::vector<Position>& traj); // met à jour la trajectoire de l'avion
    void setEtat(EtatAvion e); // met à jour l'état de l'avion
    void avancer(float temps = 1.0f); // fais avancer l'avion vers le prochain point dans la trajectoire (avance un petit peu donc ne garantit pas qu'on atteigne le point en question)
    void setParking(Parking* p);
    Parking* getParking() const;
};


class Parking {
private:
    std::string nom; // nom du parking
    int distancePiste; // distance entre le parking et la piste utile pour la priorit pour la file de dcollage
    bool occupe; // indique si le parking est occup ou pas

public:
    Parking(const std::string& nom, int distance); // constructeur
    const std::string& getNom() const; // renvoie le nom du parking
    int getDistancePiste() const; // renvoie la distance entre le parking et la piste
    bool estOccupe() const; // renvoie si le parking est occup ou non
    void occuper(); // change l'tat du parking en occup
    void liberer(); // change l'tat du parking en libre
};

class TWR {
private:
    bool pisteLibre_; // indique si la piste est libre ou pas
    std::vector<Parking> parkings_; // liste des parkings
    std::vector<Avion*> filePourDecollage_; // file des avions qui attendent de dcoller selon la priorit : "Arbitrairement, l'avion le plus loign de la piste a la priorit au dcollage"
    std::mutex mutexTWR_;
public:
    TWR(const std::vector<Parking>& parkings); // constructeur
    bool estPisteLibre() const; // vérifie si la piste est libre
    bool autoriserAtterrissage(Avion* avion); // vrifie si la piste est libre si oui on la rserve pour l'avion qui atterit sinon on refuse
    void libererPiste(); // libre la piste aprs un atterrissage ou un dcollage
    Parking* attribuerParking(Avion* avion); // assigne l'avion  un parking libre
    void gererRoulage(Avion* avion, Parking* parking); // gre toute la partie au sol de l'avion avant dcollage ou aprs un atterrissage 
    void enregistrerPourDecollage(Avion* avion); // ajoute un avion dans la file de dcollage quand il est prt
    bool autoriserDecollage(Avion* avion); // vrifie si la piste est libre si oui rserver la piste pour l'avion qui dcolle sinon on refuse
    Avion* choisirAvionPourDecollage() const; // dcide qui va dcoller en premier selon la priorit
    void retirerAvionDeDecollage(Avion* avion); // retire l'avion une fois qu'il a décollé
};



class APP {
private:
    std::vector<Avion*> avionsDansZone;//renvoie les avions présents dans la zone 
    std::queue<Avion*> fileAttenteAtterrissage;//file avec tout les avions attendant d'attérir 

    double altitudeAttente;//hauteur à laquelle les avions doivent attendre avant de pouvoir se poser 
    double rayonAttente;//"circuit d'attente" pour les avions qui doivent attérir 

    TWR* twr;//tour de controle 
    std::mutex mtx;//multithreading

public:
    APP(TWR* tour);//constructeur de la class APP

    void ajouterAvion(Avion* avion);//ajoute un avion dans la zone d'approche 
    void assignerTrajectoire(Avion* avion);//donne la trajectoire pour acceder a la piste d'aterissage 
    void mettreEnAttente(Avion* avion);//met un avion dans le circuit d'attente 
    Avion* prochainPourAtterrissage();//donne l'avion suivant dans la file d'attérissage 

    bool demanderAutorisationAtterrissage(Avion* avion);//demande à TWR si il est possible d'attérir 

    void mettreAJour();//actualise la zone d'approche

};