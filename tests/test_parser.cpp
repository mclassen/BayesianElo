#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "parser/pgn_parser.h"
#include "filters/game_filter.h"

using namespace bayeselo;

bool test_parser() {
    std::string sample = "[Event \"Casual\"]\n[White \"Alice\"]\n[Black \"Bob\"]\n[Result \"1-0\"]\n\n1. e4 e5 2. Nf3 Nc6";
    std::string path = "sample.pgn";
    {
        std::ofstream out(path);
        out << sample;
    }
    ParseOptions opts{.threads = 1};
    GameFilter filter(FilterOptions{});
    PgnParser parser(opts, filter);
    auto games = parser.parse_files({path});
    std::cout << "Parsed " << games.size() << " games\n";
    assert(games.size() == 1);
    assert(games[0].ply_count == 4);
    assert(games[0].tags.white == "Alice");
    assert(games[0].tags.result == "1-0");
    return true;
}
