cmake_minimum_required(VERSION 3.20)

# Nom de la solution globale
project(Projet1CIR2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- 1. CONFIGURATION SFML ---
set(SFML_STATIC_LIBRARIES TRUE)

if(MSVC)
    # Chemin vers SFML (Adaptez si nécessaire selon votre installation)
    set(SFML_DIR "C:/Program Files/SFML/SFML_3.0.2/lib/cmake/SFML")
    add_compile_options(/utf-8) # Pour les accents dans la console Windows
endif(MSVC)

# --- 2. DEFINITIONS ---
# Permet d'utiliser PATH_IMG dans votre code C++ pour charger les images
set(DEFS "PATH_IMG=\"${CMAKE_CURRENT_SOURCE_DIR}/Projet/img/\"")
add_compile_definitions(${DEFS})

# --- 3. DEPENDANCES ---
find_package(SFML 3 COMPONENTS Window Graphics System REQUIRED)
find_package(Threads REQUIRED)

# --- 4. EXECUTABLE ---
# On liste les fichiers en précisant qu'ils sont dans le sous-dossier "Projet/"
add_executable(Simulateur
    "Projet/main.cpp"
    "Projet/avion.cpp"
    "Projet/thread.cpp"
    "Projet/avion.hpp"
    "Projet/thread.hpp"
 "Projet/position.cpp" "Projet/parking.cpp" "Projet/twr.cpp" "Projet/app.cpp" "Projet/ccr.cpp"  "Projet/communication.cpp")

# --- 5. LIENS (LINKING) ---
target_link_libraries(Simulateur PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System
    Threads::Threads
)