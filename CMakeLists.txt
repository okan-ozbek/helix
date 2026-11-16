cmake_minimum_required(VERSION 4.2)
project(helix)

set(CMAKE_CXX_STANDARD 23)

add_library(helix_core
    src/Book.cpp
    src/Order.cpp
)
target_include_directories(helix_core PUBLIC include)

add_executable(helix
    app/main.cpp
    src/Replay.cpp
)
target_link_libraries(helix PRIVATE helix_core)

enable_testing()
add_executable(book_tests tests/book_tests.cpp)
target_link_libraries(book_tests PRIVATE helix_core)
add_test(NAME book_tests COMMAND book_tests)
