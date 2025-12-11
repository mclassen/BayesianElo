cmake_minimum_required(VERSION 3.20)
project(pgn_elo_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(bayeselo_lib
    src/util/duration.cpp
    src/util/thread_pool.cpp
    src/parser/chunk_splitter.cpp
    src/parser/pgn_parser.cpp
    src/output/terminal_output.cpp
    src/output/export_writer.cpp
    src/rating/bayeselo_solver.cpp
)

target_include_directories(bayeselo_lib PUBLIC include src)

add_executable(elo_rating src/main.cpp)
target_link_libraries(elo_rating PRIVATE bayeselo_lib)

enable_testing()
add_executable(duration_tests tests/duration_tests.cpp)
target_link_libraries(duration_tests PRIVATE bayeselo_lib)
add_test(NAME duration_tests COMMAND duration_tests)

add_executable(parser_tests tests/parser_tests.cpp)
target_link_libraries(parser_tests PRIVATE bayeselo_lib)
add_test(NAME parser_tests COMMAND parser_tests)

