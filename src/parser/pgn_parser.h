#pragma once

#include <functional>
#include <string>
#include <vector>

#include "chunk_splitter.h"
#include "../filters/game_filter.h"
#include "../../include/bayeselo/game.h"

namespace bayeselo {

struct ParseOptions {
    std::size_t threads{1};
    std::size_t chunk_size{512 * 1024};
};

class PgnParser {
public:
    PgnParser(ParseOptions opts, GameFilter filter);

    std::vector<Game> parse_files(const std::vector<std::string>& files) const;

private:
    Game parse_game(const std::vector<std::string>& lines) const;
    void parse_tags(const std::string& line, GameTags& tags) const;
    void parse_moves(const std::string& line, std::vector<std::string>& moves, std::uint32_t& ply_count) const;

    ParseOptions options_;
    GameFilter filter_;
};

}  // namespace bayeselo
