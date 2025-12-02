#pragma once
#include <thread>
#include <chrono>
#include "avion.hpp" 

void routine_avion(Avion& avion, Aeroport& depart, Aeroport& arrivee, CCR& ccr, std::vector<Aeroport*> aeroports);
void routine_twr(TWR& twr);
void routine_app(APP& app);
void routine_ccr(CCR& ccr);

void simuler_pause(int ms);
