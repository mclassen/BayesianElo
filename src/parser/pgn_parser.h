#pragma once

#include "bayeselo/filters.h"
#include "bayeselo/game.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace bayeselo {

std::vector<Game> parse_pgn_chunk(const std::filesystem::path& file, std::size_t start, std::size_t end);
bool passes_filters(const Game& game, const FilterConfig& config);

}

