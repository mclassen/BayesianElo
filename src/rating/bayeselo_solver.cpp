#include "bayeselo_solver.h"

#include <cmath>
#include <map>
#include <numeric>
#include <unordered_map>

namespace bayeselo {

namespace {
struct Pairing {
    std::size_t white;
    std::size_t black;
    double score; // 1 for white win, 0 for black, 0.5 draw
};

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
    update_stats(pairings, result.players);
    if (result.players.empty()) return result;

    std::vector<double> ratings(result.players.size(), 0.0);
    if (anchor_player) {
        ratings[index[*anchor_player]] = anchor_rating;
    }

    constexpr double k_scale = 400.0;
    constexpr double ln10_div4 = std::log(10.0) / 400.0;

    for (int iter = 0; iter < 50; ++iter) {
        std::vector<double> gradient(ratings.size(), 0.0);
        std::vector<double> hessian(ratings.size(), 1e-6);
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
            if (anchor_player && result.players[i].name == *anchor_player) continue;
            ratings[i] += gradient[i] / (hessian[i] + 1e-9) * k_scale * ln10_div4;
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

    for (std::size_t i = 0; i < result.players.size(); ++i) {
        result.players[i].rating = ratings[i];
        result.players[i].error = variance[i] > 0 ? std::sqrt(1.0 / variance[i]) * 40.0 : 0.0;
        if (result.players[i].games_played > 0) {
            result.players[i].opponent_rating_sum = result.players[i].opponent_rating_sum / result.players[i].games_played;
        }
    }

    const std::size_t n = result.players.size();
    result.los_matrix.assign(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            double diff = ratings[i] - ratings[j];
            double los = 1.0 / (1.0 + std::pow(10.0, -diff / (k_scale / 2.0)));
            result.los_matrix[i][j] = los;
        }
    }

    return result;
}

} // namespace bayeselo

