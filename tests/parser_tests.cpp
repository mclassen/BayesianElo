#include "parser/pgn_parser.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

template <typename T>
void ignore_unused(const T&) {}

int main() {
    std::string content = R"([Event "Test"]
[White "Alice"]
[Black "Bob"]
[Result "1-0"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 1-0
)";
    const std::string path = "temp_test.pgn";
    std::ofstream out(path);
    out << content;
    out.close();

    auto games = bayeselo::parse_pgn_chunk(path, 0, content.size());
    assert(games.size() == 1);
    assert(games[0].meta.white == "Alice");
    assert(games[0].meta.black == "Bob");
    assert(games[0].result.outcome == bayeselo::GameResult::Outcome::WhiteWin);
    assert(games[0].ply_count >= 6);
    std::cout << "parser tests passed\n";
    std::filesystem::remove(path);
    return 0;
}

