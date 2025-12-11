#pragma once

#include "bayeselo/game.h"
#include "bayeselo/rating_result.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace bayeselo {

class BayesEloSolver {
public:
    BayesEloSolver() = default;
    RatingResult solve(const std::vector<Game>& games, std::optional<std::string> anchor_player = std::nullopt, double anchor_rating = 0.0) const;
};

} // namespace bayeselo

