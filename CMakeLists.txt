cmake_minimum_required(VERSION 3.20)

# Nom de la solution globale
project(Projet1CIR2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- 1. CONFIGURATION SFML ---
set(SFML_STATIC_LIBRARIES TRUE)

if(MSVC)
    # Chemin vers SFML (Adaptez si nécessaire)
    set(SFML_DIR "C:/Program Files/SFML/SFML_3.0.2/lib/cmake/SFML")
    add_compile_options(/utf-8)
endif(MSVC)

# --- 2. DEPENDANCES ---
# Attention : SFML 3 utilise le composant "System" mais inclut Window/Graphics
find_package(SFML 3 COMPONENTS Window Graphics System REQUIRED)
find_package(Threads REQUIRED)

# --- 3. EXECUTABLE ---
add_executable(Simulateur
    "Projet/main.cpp"
    "Projet/avion.cpp"
    "Projet/thread.cpp"
    "Projet/avion.hpp"
    "Projet/thread.hpp"
    "Projet/position.cpp" 
    "Projet/parking.cpp" 
    "Projet/twr.cpp" 
    "Projet/app.cpp" 
    "Projet/ccr.cpp"  
    "Projet/communication.cpp")
# --- 4. LIENS (LINKING) ---
target_link_libraries(Simulateur PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System
    Threads::Threads
)

# =========================================================
# --- 5. GESTION DES RESSOURCES (IMAGES) ---
# =========================================================
# Cette commande copie le dossier "Projet/img" vers le dossier de l'exécutable (Build)
# Elle s'exécute automatiquement après la compilation.

add_custom_command(TARGET Simulateur POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Projet/img"
    "$<TARGET_FILE_DIR:Simulateur>/img"
    COMMENT "Copie du dossier img vers le dossier de l'exécutable..."
)