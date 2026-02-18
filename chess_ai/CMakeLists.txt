cmake_minimum_required(VERSION 3.20)

project(chess_ai LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(chess_ai
    src/main.cpp
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/perft.cpp
)

target_include_directories(chess_ai PRIVATE src)
