cmake_minimum_required(VERSION 3.20)

project(chess_ai LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_DIR "C:/Users/Frostbourn/Desktop/SFML/lib/cmake/SFML")

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(chess_ai
    src/main.cpp
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/perft.cpp
    src/eval.cpp
    src/search.cpp
)
target_include_directories(chess_ai PRIVATE src)
target_link_libraries(chess_ai PRIVATE SFML::System) 
add_executable(chess_gui
    src/main_gui.cpp
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/perft.cpp
    src/eval.cpp
    src/search.cpp
)
target_include_directories(chess_gui PRIVATE src)
target_link_libraries(chess_gui PRIVATE SFML::Graphics SFML::Window SFML::System)
