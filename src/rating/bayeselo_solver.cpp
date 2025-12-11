#include "bayeselo_solver.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <ranges>

namespace bayeselo {

namespace {
double expected_score(double ra, double rb) {
    return 1.0 / (1.0 + std::pow(10.0, (rb - ra) / 400.0));
}

double los(double ra, double rb, double sigma = 30.0) {
    // approximate LOS using normal distribution overlap
    double diff = ra - rb;
    double prob = 0.5 * (1.0 + std::erf(diff / (std::sqrt(2.0) * sigma)));
    return prob;
}
}

BayesEloSolver::BayesEloSolver(RatingOptions opts) : options_(std::move(opts)) {}

RatingResult BayesEloSolver::solve(const std::vector<Game>& games) const {
    std::map<std::string, std::size_t> indices;
    for (const auto& game : games) {
        indices.try_emplace(game.tags.white, indices.size());
        indices.try_emplace(game.tags.black, indices.size());
    }

    std::vector<double> ratings(indices.size(), options_.initial_rating);
    std::vector<std::size_t> counts(indices.size(), 0);

    for (std::size_t iter = 0; iter < options_.max_iterations; ++iter) {
        std::vector<double> gradients(indices.size(), 0.0);
        std::vector<double> hess(indices.size(), 0.0);

        for (const auto& game : games) {
            auto w_idx = indices[game.tags.white];
            auto b_idx = indices[game.tags.black];
            double score = 0.5;
            if (game.tags.result == "1-0") score = 1.0;
            else if (game.tags.result == "0-1") score = 0.0;
            double expect = expected_score(ratings[w_idx], ratings[b_idx]);
            double diff = score - expect;
            gradients[w_idx] += diff;
            gradients[b_idx] -= diff;
            double v = expect * (1.0 - expect);
            hess[w_idx] += v;
            hess[b_idx] += v;
            counts[w_idx]++;
            counts[b_idx]++;
        }

        double max_change = 0.0;
        for (std::size_t i = 0; i < ratings.size(); ++i) {
            double step = gradients[i] / (hess[i] + 1e-6);
            ratings[i] += 32.0 * step;
            max_change = std::max(max_change, std::abs(step));
        }
        if (max_change < options_.tolerance) break;
    }

    if (options_.anchor_player) {
        auto it = indices.find(*options_.anchor_player);
        if (it != indices.end()) {
            double delta = options_.anchor_rating - ratings[it->second];
            for (auto& r : ratings) r += delta;
        }
    }

    RatingResult result;
    result.players.resize(indices.size());
    for (const auto& [name, idx] : indices) {
        PlayerStats stats;
        stats.name = name;
        stats.rating = ratings[idx];
        stats.error = 60.0 / std::sqrt(std::max<std::size_t>(1, counts[idx]));
        stats.games = counts[idx];
        result.players[idx] = stats;
    }

    result.los_matrix.assign(indices.size(), std::vector<double>(indices.size(), 0.5));
    for (const auto& [a_name, a_idx] : indices) {
        for (const auto& [b_name, b_idx] : indices) {
            result.los_matrix[a_idx][b_idx] = los(ratings[a_idx], ratings[b_idx]);
        }
    }

    std::ranges::sort(result.players, std::ranges::greater{}, [](const PlayerStats& s) { return s.rating; });
    return result;
}

}  // namespace bayeselo
