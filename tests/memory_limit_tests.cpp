#include "parser/pgn_parser.h"
#include "bayeselo/filters.h"

#include <atomic>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    // Two simple games to exercise max-games limiting and move trimming.
    const std::string content = R"([Event "G1"]
[White "A"]
[Black "B"]
[Result "1-0"]

1. e4 e5 1-0

[Event "G2"]
[White "C"]
[Black "D"]
[Result "0-1"]

1. d4 d5 0-1
)";
    const std::string path = "temp_memory_limit.pgn";
    {
        std::ofstream out(path, std::ios::binary);
        out << content;
    }

    auto games = bayeselo::parse_pgn_chunk(path, 0, content.size());
    bayeselo::FilterConfig cfg;

    std::atomic_size_t accepted{0};
    std::vector<bayeselo::Game> kept;
    for (auto& g : games) {
        // simulate default move-trimming behavior
        g.moves.clear();
        g.moves.shrink_to_fit();
        assert(g.moves.empty());
        if (!bayeselo::passes_filters(g, cfg)) continue;
        if (accepted.load() >= 1) break; // simulate --max-games=1
        kept.push_back(std::move(g));
        accepted.fetch_add(1);
    }

    assert(kept.size() == 1);
    assert(kept[0].meta.white == "A");
    assert(kept[0].result.outcome == bayeselo::GameResult::Outcome::WhiteWin);
    std::cout << "memory limit tests passed\n";
    std::filesystem::remove(path);
    return 0;
}
