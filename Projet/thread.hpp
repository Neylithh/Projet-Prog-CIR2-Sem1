#pragma once
#include <thread>
#include <chrono>
// On inclut SEULEMENT avion.hpp car tu m'as dit que toutes les classes (CCR, Aeroport...) y sont.
#include "avion.hpp" 

// Déclarations des routines (Promesses)
void routine_avion(Avion& avion, Aeroport& depart, Aeroport& arrivee, CCR& ccr);
void routine_twr(TWR& twr);
void routine_app(APP& app);
void routine_ccr(CCR& ccr); // <--- La promesse est ici

void simuler_pause(int ms);