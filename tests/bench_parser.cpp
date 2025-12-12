#include "parser/chunk_splitter.h"
#include "parser/pgn_parser.h"
#include "bayeselo/size_parse.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

std::size_t env_size_bytes(const char* name, std::size_t fallback) {
    if (const char* val = std::getenv(name)) {
        return bayeselo::parse_size_or(val, fallback);
    }
    return fallback;
}

struct GameTemplate {
    std::string white;
    std::string black;
    std::string result;
    std::string termination;
    std::string time_control; // may be empty
    std::string moves;
};

void write_synthetic_pgn(const std::filesystem::path& path, std::size_t target_bytes) {
    // Deterministic but varied set of games to stress tags and move parsing.
    const std::vector<GameTemplate> variants{
        {"Alpha", "Beta", "1-0", "Normal", "5m+3", "1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 1-0"},
        {"Gamma", "Delta", "0-1", "Time forfeit", "3m+2", "1. d4 Nf6 2. c4 e6 3. Nc3 Bb4 0-1"},
        {"Epsilon", "Zeta", "1/2-1/2", "Abandoned", "60", "1. c4 e5 2. Nc3 Nf6 3. g3 d5 4. cxd5 Nxd5 1/2-1/2"},
        {"Eta", "Theta", "*", "", "", "1. Nf3 d5 2. g3 {comment} 2... c5 (2...Nf6) 3. Bg2 *"},
    };

    std::ofstream out(path, std::ios::binary);
    std::size_t idx = 0;
    while (out.tellp() < static_cast<std::streamoff>(target_bytes)) {
        const auto& g = variants[idx % variants.size()];
        out << "[Event \"Bench " << idx << "\"]\n"
            << "[Site \"Local\"]\n"
            << "[White \"" << g.white << "\"]\n"
            << "[Black \"" << g.black << "\"]\n"
            << "[Result \"" << g.result << "\"]\n";
        if (!g.termination.empty()) {
            out << "[Termination \"" << g.termination << "\"]\n";
        }
        if (!g.time_control.empty()) {
            out << "[TimeControl \"" << g.time_control << "\"]\n";
        }
        out << "\n" << g.moves << "\n\n";
        ++idx;
    }
    out.flush();
}

}  // namespace

int main(int argc, char** argv) {
    bool keep_file = false;
    std::size_t target_bytes = 10 * 1024ull * 1024ull; // default 10 MB
    std::size_t chunk_bytes = 1 * 1024ull * 1024ull;  // default 1 MiB

    if (const char* arg = std::getenv("BENCH_KEEP_FILE")) {
        if (std::string(arg) == "1") keep_file = true;
    }
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--keep-file") {
            keep_file = true;
        } else if (std::string_view(argv[i]).rfind("--generate-pgn-size=", 0) == 0) {
            auto val = std::string_view(argv[i]).substr(std::string_view("--generate-pgn-size=").size());
            target_bytes = bayeselo::parse_size_or(val, target_bytes);
        } else if (std::string_view(argv[i]).rfind("--chunk-size=", 0) == 0) {
            auto val = std::string_view(argv[i]).substr(std::string_view("--chunk-size=").size());
            chunk_bytes = bayeselo::parse_size_or(val, chunk_bytes);
        }
    }

    // Env overrides (bytes or with suffixes)
    target_bytes = env_size_bytes("BENCH_PGN_MB", target_bytes);
    chunk_bytes = env_size_bytes("BENCH_CHUNK_BYTES", chunk_bytes);

    auto tmp = std::filesystem::temp_directory_path() / "bench_parser.pgn";
    write_synthetic_pgn(tmp, target_bytes);

    const auto start = std::chrono::steady_clock::now();
    auto chunks = bayeselo::split_pgn_file(tmp, chunk_bytes);
    std::size_t games = 0;
    for (const auto& c : chunks) {
        auto parsed = bayeselo::parse_pgn_chunk(c.file, c.start_offset, c.end_offset);
        games += parsed.size();
    }
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;

    double mb = static_cast<double>(target_bytes) / (1024.0 * 1024.0);
    double mb_per_sec = mb / elapsed.count();
    std::cout << "Parsed " << games << " games from ~" << mb << " MB in "
              << elapsed.count() << "s (" << mb_per_sec << " MB/s)\n";

    if (!keep_file) {
        std::filesystem::remove(tmp);
    } else {
        std::cout << "Keeping benchmark file at: " << tmp << "\n";
    }
    return 0;
}
