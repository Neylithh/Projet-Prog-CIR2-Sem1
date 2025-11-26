#pragma once
#include <thread>
#include <chrono>
#include "avion.hpp"

// Routines des threads
void routine_avion(Avion& avion, APP& app, TWR& twr);
void routine_twr(TWR& twr);
void routine_app(APP& app); // Nouvelle routine pour l'APP

void simuler_pause(int ms);