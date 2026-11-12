cmake_minimum_required(VERSION 4.2)
project(helix)

set(CMAKE_CXX_STANDARD 23)

add_executable(
    helix src/main.cpp
    include/SimpleBook.h
    include/Event.h
        include/Replay.h
)
