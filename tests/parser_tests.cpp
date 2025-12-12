#include "parser/pgn_parser.h"
#include "parser/chunk_splitter.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>

int main() {
    const std::string path = "temp_test.pgn";
    const std::string chunk_path = "temp_chunk.pgn";
    struct TempFileGuard {
        std::filesystem::path path;
        std::filesystem::path chunk_path;
        ~TempFileGuard() {
            std::error_code ec;
            std::filesystem::remove(path, ec);
            std::filesystem::remove(chunk_path, ec);
        }
    } guard{path, chunk_path};
    auto fail = [](const std::string& msg) {
        std::cerr << msg << "\n";
        return 1;
    };

    std::string content = R"([Event "Test"]
[White "Alice"]
[Black "Bob"]
[Result "1-0"]
[Termination "Normal"]
[TimeControl "5m+3"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 1-0
)";
    std::ofstream out(path, std::ios::binary);
    out << content;
    out.close();

    std::vector<bayeselo::Game> games;
    try {
        auto parsed = bayeselo::parse_pgn_chunk(path, 0, content.size());
        if (!parsed) return fail("parse_pgn_chunk returned nullopt");
        games = std::move(*parsed);
    } catch (const std::exception& ex) {
        return fail(std::string("parse_pgn_chunk threw: ") + ex.what());
    }
    if (games.size() != 1) return fail("expected 1 game parsed, got " + std::to_string(games.size()));
    const auto& game = games[0];
    if (game.meta.white != "Alice") return fail("expected white Alice, got " + game.meta.white);
    if (game.meta.black != "Bob") return fail("expected black Bob, got " + game.meta.black);
    if (game.result.outcome != bayeselo::GameResult::Outcome::WhiteWin) return fail("outcome not parsed as WhiteWin");
    if (game.ply_count < 6) return fail("ply count too low: " + std::to_string(game.ply_count));
    if (!game.estimated_duration_seconds) return fail("missing estimated duration");
    if (std::abs(*game.estimated_duration_seconds - 300.0) > 1e-6) return fail("expected 300s estimate, got " + std::to_string(*game.estimated_duration_seconds));
    bayeselo::FilterConfig config;
    config.termination = "normal";
    if (!bayeselo::passes_filters(game, config)) return fail("termination filter rejected valid game");
    auto filtered = game;
    filtered.result.termination = std::nullopt;
    if (bayeselo::passes_filters(filtered, config)) return fail("termination filter accepted missing termination");

    // Chunk splitter should clamp to EOF even without trailing Event
    const std::string single_game = R"([Event "Solo"]
[White "X"]
[Black "Y"]
[Result "1-0"]

1. e4 e5 2. Nf3 Nc6 1-0
)";
    {
        std::ofstream f(chunk_path, std::ios::binary);
        f << single_game;
    }
    auto chunks = bayeselo::split_pgn_file(chunk_path, 16);
    std::size_t size = 0;
    {
        std::ifstream fin(chunk_path, std::ios::binary | std::ios::ate);
        size = static_cast<std::size_t>(fin.tellg());
    }
    if (chunks.empty()) return fail("chunk splitter produced no chunks");
    if (chunks.back().end_offset != size) return fail("chunk end offset mismatch: expected " + std::to_string(size) + ", got " + std::to_string(chunks.back().end_offset));
    std::cout << "parser tests passed\n";
    return 0;
}
