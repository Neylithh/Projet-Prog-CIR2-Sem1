#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <queue>
#include <mutex>
#pragma once

class Avion {
private:
    std::string nom_; // nom de l'avion
    float vitesse_; // vitesse de l'avion
    float carburant_; // niveau de carburant
    float conso_; // consommation de carburant 
    Position pos_; // position de l'avion
    std::vector<Position> trajectoire_;  // liste de points � suivre
    EtatAvion etat_; // �tat de l'avion

public:
    Avion(std::string n, float v, float c, float conso, Position pos, const std::vector<Position>& traj); // constructeur de l'avion
    std::string getNom() const; // renvoie le nom de l'avion
    float getVitesse() const; // renvoie la vitesse de l'avion
    float getCarburant() const; // renvoie le carburant de l'avion
    float getConsommation() const; // renvoie la consommaiton de l'avion
    Position getPosition() const; // renvoie la position de l'avion
    const std::vector<Position>& getTrajectoire() const; // renvoie la trajectoire de l'avion
    EtatAvion getEtat() const; // renvoie l'�tat de l'avion
    void setPosition(const Position& p); // met � jour la position de l'avion
    void setTrajectoire(const std::vector<Position>& traj); // met � jour la trajectoire de l'avion
    void setEtat(EtatAvion e); // met � jour l'�tat de l'avion
    void avancer(float temps = 1.0f); // fais avancer l'avion vers le prochain point dans la trajectoire (avance un petit peu donc ne garantit pas qu'on atteigne le point en question)
};

class Position {
private:
    float x_; // position x
    float y_; // position y
    float altitude_; // altitude

public:
    Position(float x = 0, float y = 0, float z = 0); // constructeur
    float getX() const; // renvoie la coordonn�e x
    float getY() const; // renvoie la coordonn�e y
    float getAltitude() const; // renvoie l'altitude 
    void setPosition(float x, float y, float alt); // met la position aux coordonn�es en param�tres
};

enum class EtatAvion { // liste des diff�rents �tats pour un avion
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
    int distancePiste; // distance entre le parking et la piste utile pour la priorit� pour la file de d�collage
    bool occupe; // indique si le parking est occup� ou pas

public:
    Parking(const std::string& nom, int distance); // constructeur
    const std::string& getNom() const; // renvoie le nom du parking
    int getDistancePiste() const; // renvoie la distance entre le parking et la piste
    bool estOccupe() const; // renvoie si le parking est occup� ou non
    void occuper(); // change l'�tat du parking en occup�
    void liberer(); // change l'�tat du parking en libre
};

class TWR {
private :
    bool pisteLibre; // indique si la piste est libre ou pas
    std::vector<Parking> parkings; // liste des parkings
    std::vector<Avion*> filePourDecollage; // file des avions qui attendent de d�coller selon la priorit� : "Arbitrairement, l'avion le plus �loign� de la piste a la priorit� au d�collage"
public:
    bool autoriserAtterrissage(Avion* avion); // v�rifie si la piste est libre si oui on la r�serve pour l'avion qui atterit sinon on refuse
    void libererPiste(); // lib�re la piste apr�s un atterrissage ou un d�collage
    Parking* attribuerParking(Avion* avion); // assigne l'avion � un parking libre
    void gererRoulage(Avion* avion, Parking* parking); // g�re toute la partie au sol de l'avion avant d�collage ou apr�s un atterrissage 
    void enregistrerPourDecollage(Avion* avion); // ajoute un avion dans la file de d�collage quand il est pr�t
    bool autoriserDecollage(Avion* avion); // v�rifie si la piste est libre si oui r�server la piste pour l'avion qui d�colle sinon on refuse
    Avion* choisirAvionPourDecollage() const; // d�cide qui va d�coller en premier selon la priorit�
};


class APP  {
private:
    std::vector<Avion*> avionsDansZone;
    std::queue<Avion*> fileAttenteAtterrissage;

    double altitudeAttente;
    double rayonAttente;

    TWR* twr;
    std::mutex mtx;

public:
    APP(TWR* tour);

    void ajouterAvion(Avion* avion);
    void assignerTrajectoire(Avion* avion);
    void mettreEnAttente(Avion* avion);
    Avion* prochainPourAtterrissage();

    bool demanderAutorisationAtterrissage(Avion* avion);

    void mettreAJour();  

};
