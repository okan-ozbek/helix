cmake_minimum_required(VERSION 4.2)
project(helix)

set(CMAKE_CXX_STANDARD 23)

add_executable(
    helix app/main.cpp
    include/Orderbook.h
    include/Event.h
    include/Replay.h
    include/Order.h
    src/Replay.cpp
    src/Orderbook.cpp
    src/Order.cpp
)
