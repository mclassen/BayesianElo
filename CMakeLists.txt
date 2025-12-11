cmake_minimum_required(VERSION 3.20)
project(pgn_elo_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(elo_rating
    src/main.cpp
    src/parser/pgn_parser.cpp
    src/parser/chunk_splitter.cpp
    src/filters/game_filter.cpp
    src/rating/bayeselo_solver.cpp
    src/output/terminal_output.cpp
    src/output/export_writer.cpp
    src/util/thread_pool.cpp)

target_include_directories(elo_rating PRIVATE include src)

add_executable(tests
    tests/test_main.cpp
    tests/test_parser.cpp
    tests/test_filters.cpp
    tests/test_rating.cpp
    src/parser/pgn_parser.cpp
    src/parser/chunk_splitter.cpp
    src/filters/game_filter.cpp
    src/rating/bayeselo_solver.cpp
    src/util/thread_pool.cpp)

target_include_directories(tests PRIVATE include src)

enable_testing()
add_test(NAME unit_tests COMMAND tests)

message(STATUS "Inspired by the BayesElo rating system created by Rémi Coulom.")
