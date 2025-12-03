#include "avion.hpp"
#include <filesystem>

std::ostream& operator<<(std::ostream& os, const Position& pos) {
    os << "(" << (int)pos.x_ << ", " << (int)pos.y_ << ", Alt:" << (int)pos.altitude_ << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Avion& avion) {
    os << "[" << avion.getNom() << "  Carburant : " << (int)avion.getCarburant() << "]";
    return os;
}

Logger::Logger() : premierElement_(true) {
    std::filesystem::path cheminFichierSource = __FILE__;
    std::filesystem::path dossierProjet = cheminFichierSource.parent_path();
    std::filesystem::path cheminLog = dossierProjet / "img" / "logs.json";
    fichier_.open(cheminLog);
    if (fichier_.is_open()) {
        fichier_ << "[\n";
    }
    else {
        std::cerr << "Impossible de creer img/logs.json\n";
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
        fichier_ << "    \"Controleur\": \"" << acteur << "\",\n";
        fichier_ << "    \"Action\": \"" << action << "\",\n";
        fichier_ << "    \"Details\": \"" << details << "\"\n";
        fichier_ << "  }";
        premierElement_ = false;
    }
}
