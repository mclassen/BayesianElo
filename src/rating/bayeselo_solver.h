#pragma once

#include "bayeselo/game.h"
#include "bayeselo/rating_result.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bayeselo {

struct Pairing {
    std::size_t white{};
    std::size_t black{};
    double score{0.5}; // 1 = white win, 0 = black win, 0.5 draw
};

class BayesEloSolver {
public:
    BayesEloSolver() = default;
    RatingResult solve(const std::vector<Game>& games, std::optional<std::string> anchor_player = std::nullopt, double anchor_rating = 0.0) const;
    RatingResult solve(const std::vector<Pairing>& pairings, const std::vector<std::string>& names, std::optional<std::string> anchor_player = std::nullopt, double anchor_rating = 0.0) const;
};

} // namespace bayeselo
