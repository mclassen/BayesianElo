#include "bayeselo_solver.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <optional>
#include <unordered_map>

namespace bayeselo {

namespace {

std::vector<Pairing> build_pairings(const std::vector<Game>& games, std::vector<PlayerStats>& players, std::unordered_map<std::string, std::size_t>& index) {
    std::vector<Pairing> pairings;
    for (const auto& game : games) {
        auto ensure = [&](const std::string& name) {
            auto it = index.find(name);
            if (it == index.end()) {
                std::size_t idx = players.size();
                index[name] = idx;
                players.push_back(PlayerStats{name});
                return idx;
            }
            return it->second;
        };
        auto w = ensure(game.meta.white);
        auto b = ensure(game.meta.black);
        double score = 0.5;
        if (game.result.outcome == GameResult::Outcome::WhiteWin) score = 1.0;
        else if (game.result.outcome == GameResult::Outcome::BlackWin) score = 0.0;
        pairings.push_back(Pairing{w, b, score});
    }
    return pairings;
}

void update_stats(const std::vector<Pairing>& pairings, std::vector<PlayerStats>& players) {
    for (const auto& p : pairings) {
        auto& white = players[p.white];
        auto& black = players[p.black];
        white.games_played++;
        black.games_played++;
        white.score_sum += p.score;
        black.score_sum += 1.0 - p.score;
        if (p.score == 0.5) {
            white.draws += 1;
            black.draws += 1;
        }
    }
}

} // namespace

RatingResult BayesEloSolver::solve(const std::vector<Game>& games, std::optional<std::string> anchor_player, double anchor_rating) const {
    RatingResult result;
    std::unordered_map<std::string, std::size_t> index;
    auto pairings = build_pairings(games, result.players, index);
    std::vector<std::string> names;
    names.reserve(result.players.size());
    for (const auto& p : result.players) names.push_back(p.name);
    if (names.size() <= 1) {
        result.los_matrix.assign(result.players.size(), std::vector<double>(result.players.size(), 1.0));
        return result; // Single-player data is not meaningful for Elo.
    }
    return solve(pairings, names, anchor_player, anchor_rating);
}

RatingResult BayesEloSolver::solve(const std::vector<Pairing>& pairings, const std::vector<std::string>& names, std::optional<std::string> anchor_player, double anchor_rating) const {
    RatingResult result;
    result.players.reserve(names.size());
    for (const auto& n : names) {
        result.players.push_back(PlayerStats{n});
    }
    if (result.players.empty()) return result;

    update_stats(pairings, result.players);

    std::vector<double> ratings(result.players.size(), 0.0);
    std::optional<std::size_t> anchor_index;
    if (anchor_player) {
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (names[i] == *anchor_player) {
                anchor_index = i;
                ratings[i] = anchor_rating;
                break;
            }
        }
    }

    // Solver constants mirror classic Elo/BayesElo conventions.
    constexpr double k_scale = 400.0; // 400 pts difference ≈ 10:1 odds.
    const double ln10_div4 = std::log(10.0) / k_scale; // ln(10)/400 used in gradient update.
    constexpr int max_iterations = 50; // Fixed iteration cap for convergence.
    constexpr double hessian_reg = 1e-6; // Diagonal regularizer to avoid singular Hessian.
    constexpr double denom_reg = 1e-9;   // Extra guard for divide-by-zero.
    constexpr double error_scale = 40.0; // Error is reported on ~k_scale/10 granularity.
    constexpr double min_variance = 1e-6; // Caps error for near-zero information games.

    for (int iter = 0; iter < max_iterations; ++iter) {
        std::vector<double> gradient(ratings.size(), 0.0);
        std::vector<double> hessian(ratings.size(), hessian_reg);
        for (const auto& p : pairings) {
            double diff = ratings[p.white] - ratings[p.black];
            double expected = 1.0 / (1.0 + std::pow(10.0, -diff / k_scale));
            double variance = expected * (1.0 - expected);
            gradient[p.white] += (p.score - expected);
            gradient[p.black] -= (p.score - expected);
            hessian[p.white] += variance;
            hessian[p.black] += variance;
        }
        for (std::size_t i = 0; i < ratings.size(); ++i) {
            if (anchor_index && i == *anchor_index) continue;
            ratings[i] += gradient[i] / (hessian[i] + denom_reg) * k_scale * ln10_div4;
        }
    }

    // Error estimate simplistic: based on inverse hessian diagonal
    std::vector<double> variance(ratings.size(), 0.0);
    for (const auto& p : pairings) {
        double diff = ratings[p.white] - ratings[p.black];
        double expected = 1.0 / (1.0 + std::pow(10.0, -diff / k_scale));
        double var = expected * (1.0 - expected);
        variance[p.white] += var;
        variance[p.black] += var;
    }

    std::vector<double> opponent_rating_sum(ratings.size(), 0.0);
    for (const auto& p : pairings) {
        opponent_rating_sum[p.white] += ratings[p.black];
        opponent_rating_sum[p.black] += ratings[p.white];
    }

    for (std::size_t i = 0; i < result.players.size(); ++i) {
        result.players[i].rating = ratings[i];
        result.players[i].error = variance[i] > 0 ? std::sqrt(1.0 / std::max(variance[i], min_variance)) * error_scale : 0.0;
        result.players[i].opponent_rating_sum = opponent_rating_sum[i]; // Sum of opponents' ratings across games.
    }

    const std::size_t n = result.players.size();
    result.los_matrix.assign(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            double diff = ratings[i] - ratings[j];
            // LOS uses half-scale (k_scale / 2.0) to approximate P(r_i > r_j): a BayesElo convention to make LOS more discriminative.
            double los = 1.0 / (1.0 + std::pow(10.0, -diff / (k_scale / 2.0)));
            result.los_matrix[i][j] = los;
        }
    }

    // Sort by rating while keeping LOS aligned
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return result.players[a].rating > result.players[b].rating;
    });

    std::vector<PlayerStats> sorted_players;
    sorted_players.reserve(n);
    std::vector<std::vector<double>> sorted_los(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        sorted_players.push_back(result.players[order[i]]);
        for (std::size_t j = 0; j < n; ++j) {
            sorted_los[i][j] = result.los_matrix[order[i]][order[j]];
        }
    }
    result.players = std::move(sorted_players);
    result.los_matrix = std::move(sorted_los);

    return result;
}

} // namespace bayeselo
