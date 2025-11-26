#include "avion.hpp"
#include "thread.hpp"

std::vector<Avion*> flotte; // Pour garder les pointeurs vivants

int main() {
    std::srand(time(0));

    std::cout << "===========================================\n";
    std::cout << "   CONTROLE AERIEN EUROPE (CCR SYSTEM)     \n";
    std::cout << "===========================================\n";

    // 1. Initialisation CCR
    CCR ccr;
    std::thread tCCR(routine_ccr, std::ref(ccr));

    // 2. Création Aéroports (Positions distantes)
    Aeroport cdg("CDG-Paris", Position(0, 0, 0));
    Aeroport lhr("LHR-London", Position(-20000, 30000, 0));
    Aeroport mad("MAD-Madrid", Position(-10000, -40000, 0));

    std::vector<Aeroport*> aeres = { &cdg, &lhr, &mad };
    std::vector<std::thread> threadsInfra;

    // 3. Threads Infra
    for (auto* apt : aeres) {
        threadsInfra.emplace_back(routine_twr, std::ref(*(apt->twr)));
        threadsInfra.emplace_back(routine_app, std::ref(*(apt->app)));
        std::cout << "[INIT] " << apt->nom << " operationnel.\n";
    }

    // 4. Générateur de Trafic
    int id = 100;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Random Dep/Arr
        Aeroport* dep = aeres[rand() % aeres.size()];
        Aeroport* arr = aeres[rand() % aeres.size()];
        if (dep == arr) continue;

        // Nouvel avion
        std::string nom = "VOL-" + std::to_string(id++);
        Avion* a = new Avion(nom, 1000.f, 10000.f, 5.f, dep->position);
        flotte.push_back(a);

        // Thread Pilote détaché
        std::thread t(routine_avion, std::ref(*a), std::ref(*dep), std::ref(*arr), std::ref(ccr));
        t.detach();

        std::cout << "--- [PLAN DE VOL] " << nom << " : " << dep->nom << " -> " << arr->nom << " ---\n";
    }

    // cleanup (unreached)
    if (tCCR.joinable()) tCCR.join();
    for (auto& t : threadsInfra) if (t.joinable()) t.join();

    return 0;
}