#include "avion.hpp"

// ================= IMPLEMENTATION LOGGER =================

// ================= SURCHARGE << POUR POSITION =================
std::ostream& operator<<(std::ostream& os, const Position& pos) {
    // On accède directement à x_, y_ car on est "friend"
    os << "(" << (int)pos.x_ << ", " << (int)pos.y_ << ", Alt:" << (int)pos.altitude_ << ")";
    return os;
}

// ================= SURCHARGE << POUR AVION =================
std::ostream& operator<<(std::ostream& os, const Avion& avion) {
    // Affiche par exemple : [AIRFRANCE-1 | Carburant: 85%]
    os << "[" << avion.getNom() << " | Carburant:" << (int)avion.getCarburant() << "]";
    return os;
}

// ... Tes autres méthodes (Logger::log, etc.) ...
Logger::Logger() : premierElement_(true) {
    fichier_.open("logs.json");
    if (fichier_.is_open()) {
        fichier_ << "[\n";
    }
}

Logger::~Logger() {
    if (fichier_.is_open()) {
        fichier_ << "\n]";
        fichier_.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& acteur, const std::string& action, const std::string& details) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fichier_.is_open()) {
        if (!premierElement_) fichier_ << ",\n";
        fichier_ << "  {\n";
        fichier_ << "    \"acteur\": \"" << acteur << "\",\n";
        fichier_ << "    \"action\": \"" << action << "\",\n";
        fichier_ << "    \"details\": \"" << details << "\"\n";
        fichier_ << "  }";
        premierElement_ = false;
    }
}