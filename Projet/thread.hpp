#include <thread>
#include <chrono>
#include "avion.hpp"

void routine_avion(Avion& avion, APP& app, TWR& twr);
void simuler_pause(int ms);
void routine_twr(TWR& twr);