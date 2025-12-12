#pragma once

#include "game.h"
#include <string>
#include <vector>

namespace bayeselo {

struct RatingResult {
    std::vector<PlayerStats> players;
    std::vector<std::vector<double>> los_matrix;
};

} // namespace bayeselo
