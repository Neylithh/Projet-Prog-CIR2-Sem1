#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>

#include "avion.hpp"
#include "thread.hpp"

std::vector<std::thread> threads_infra;

int main() {
    std::srand(static_cast<unsigned int>(time(NULL)));

    std::cout << "SIMULATEUR ESPACE AERIEN \n";


    CCR ccr;

    Aeroport lille("Lille", Position(0, 0, 0), 20000.0f);
    Aeroport paris("Paris-CDG", Position(90000, 50000, 0), 20000.0f);
    Aeroport marseille("Marseille", Position(300000, 70000, 0), 20000.0f);

    threads_infra.emplace_back(routine_ccr, std::ref(ccr));
    threads_infra.emplace_back(routine_twr, std::ref(*lille.twr));
    threads_infra.emplace_back(routine_app, std::ref(*lille.app));
    std::cout << "Aeroport " << lille.nom << " pret.\n";
    threads_infra.emplace_back(routine_twr, std::ref(*paris.twr));
    threads_infra.emplace_back(routine_app, std::ref(*paris.app));
    std::cout << "Aeroport " << paris.nom << " pret.\n";
    threads_infra.emplace_back(routine_twr, std::ref(*marseille.twr));
    threads_infra.emplace_back(routine_app, std::ref(*marseille.app));
    std::cout << "Aeroport " << marseille.nom << " pret.\n";

    int idAvion = 1;
    std::vector<Avion*> flotte;

    std::vector<Aeroport*> aeroports = { &lille, &paris, &marseille };

    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::string nom = "AIRFRANCE-" + std::to_string(idAvion++);

        int idxDepart = std::rand() % aeroports.size();
        int idxDest;
        do {
            idxDest = std::rand() % aeroports.size();
        } while (idxDest == idxDepart);

        Aeroport* depart = aeroports[idxDepart];
        Aeroport* destination = aeroports[idxDest];

        Position posDepart = depart->position;
        posDepart.setPosition(posDepart.getX(), posDepart.getY() - 5000, 10000);

        Avion* nouvelAvion = new Avion(nom, 400.f, 10.f, 5000.f, 2.f, 5000.f, posDepart);
        nouvelAvion->setDestination(destination);

        flotte.push_back(nouvelAvion);

        ccr.prendreEnCharge(nouvelAvion);

        std::thread tPilote(routine_avion, std::ref(*nouvelAvion), std::ref(*depart), std::ref(*destination), std::ref(ccr), std::ref(aeroports));
        tPilote.detach();

        double dist = nouvelAvion->getPosition().distance(destination->position);
    }

    for (auto& t : threads_infra) {
        if (t.joinable()) t.join();
    }

    for (auto avion : flotte) {
        delete avion;
    }

    return 0;
}
