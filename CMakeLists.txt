cmake_minimum_required(VERSION 3.20)
project(Projet1CIR2)
set (CMAKE_CXX_STANDARD 20)

set(SFML_STATIC_LIBRARIES TRUE)

if(MSVC)
    set(SFML_DIR "C:/Program Files/SFML/SFML_3.0.2/lib/cmake/SFML")
    add_compile_options(/utf-8)
endif(MSVC)

    set(DEFS "PATH_IMG=\"${CMAKE_CURRENT_SOURCE_DIR}/img/\"")
    add_compile_definitions(${DEFS})

find_package(SFML 3 COMPONENTS Window Graphics System REQUIRED)
link_libraries(SFML::Graphics SFML::Window SFML::System)



add_executable (Projet "Projet/main.cpp" "Projet/avion.cpp" "Projet/thread.cpp")
