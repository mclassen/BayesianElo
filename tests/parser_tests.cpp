#include "parser/pgn_parser.h"
#include "parser/chunk_splitter.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

template <typename T>
void ignore_unused(const T&) {}

int main() {
    std::string content = R"([Event "Test"]
[White "Alice"]
[Black "Bob"]
[Result "1-0"]
[Termination "Normal"]
[TimeControl "5m+3"]

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
    assert(games[0].estimated_duration_seconds);
    assert(*games[0].estimated_duration_seconds == 300.0);
    bayeselo::FilterConfig config;
    config.termination = "normal";
    assert(bayeselo::passes_filters(games[0], config));
    games[0].result.termination = std::nullopt;
    assert(!bayeselo::passes_filters(games[0], config));

    // Chunk splitter should clamp to EOF even without trailing Event
    const std::string single_game = R"([Event "Solo"]
[White "X"]
[Black "Y"]
[Result "1-0"]

1. e4 e5 2. Nf3 Nc6 1-0
)";
    const std::string chunk_path = "temp_chunk.pgn";
    {
        std::ofstream f(chunk_path, std::ios::binary);
        f << single_game;
    }
    auto chunks = bayeselo::split_pgn_file(chunk_path, 16);
    std::ifstream fin(chunk_path, std::ios::binary | std::ios::ate);
    auto size = static_cast<std::size_t>(fin.tellg());
    assert(!chunks.empty());
    assert(chunks.back().end_offset == size);
    std::filesystem::remove(chunk_path);
    std::cout << "parser tests passed\n";
    std::filesystem::remove(path);
    return 0;
}
