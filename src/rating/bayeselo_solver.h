#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../../include/bayeselo/game.h"

namespace bayeselo {

struct RatingOptions {
    std::optional<std::string> anchor_player;
    double anchor_rating{0.0};
    double initial_rating{0.0};
    std::size_t max_iterations{200};
    double tolerance{1e-6};
};

class BayesEloSolver {
public:
    explicit BayesEloSolver(RatingOptions opts = {});
    RatingResult solve(const std::vector<Game>& games) const;

private:
    RatingOptions options_;
};

}  // namespace bayeselo
