cmake_minimum_required(VERSION 3.20)
project(pgn_elo_cli VERSION 0.1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if (NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()

configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h @ONLY)

add_library(bayeselo_lib
    src/util/duration.cpp
    src/util/size_parse.cpp
    src/util/thread_pool.cpp
    src/parser/chunk_splitter.cpp
    src/parser/pgn_parser.cpp
    src/output/terminal_output.cpp
    src/output/export_writer.cpp
    src/rating/bayeselo_solver.cpp
)

target_include_directories(bayeselo_lib PUBLIC include src)
target_include_directories(bayeselo_lib PUBLIC ${CMAKE_BINARY_DIR}/generated)

add_executable(elo_rating src/main.cpp)
target_link_libraries(elo_rating PRIVATE bayeselo_lib)
target_include_directories(elo_rating PRIVATE ${CMAKE_BINARY_DIR}/generated)

enable_testing()
add_executable(duration_tests tests/duration_tests.cpp)
target_link_libraries(duration_tests PRIVATE bayeselo_lib)
add_test(NAME duration_tests COMMAND duration_tests)

add_executable(parser_tests tests/parser_tests.cpp)
target_link_libraries(parser_tests PRIVATE bayeselo_lib)
add_test(NAME parser_tests COMMAND parser_tests)

add_executable(rating_tests tests/rating_tests.cpp)
target_link_libraries(rating_tests PRIVATE bayeselo_lib)
add_test(NAME rating_tests COMMAND rating_tests)

add_executable(size_parse_tests tests/size_parse_tests.cpp)
target_link_libraries(size_parse_tests PRIVATE bayeselo_lib)
add_test(NAME size_parse_tests COMMAND size_parse_tests)

add_executable(bench_parser tests/bench_parser.cpp)
target_link_libraries(bench_parser PRIVATE bayeselo_lib)
add_test(NAME bench_parser COMMAND bench_parser)
set_tests_properties(bench_parser PROPERTIES DISABLED TRUE)
set_target_properties(bench_parser PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_executable(memory_limit_tests tests/memory_limit_tests.cpp)
target_link_libraries(memory_limit_tests PRIVATE bayeselo_lib)
add_test(NAME memory_limit_tests COMMAND memory_limit_tests)
