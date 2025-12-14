#include "bayeselo/fastchess_stats.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace bayeselo {

namespace {

double normal_cdf(double z) {
    // Φ(z) = 0.5 * erfc(-z / sqrt(2))
    constexpr double inv_sqrt2 = 0.70710678118654752440;
    return 0.5 * std::erfc(-z * inv_sqrt2);
}

double score_to_elo_diff(double score) {
    // Match fastchess EloWDL convention: -400 * log10(1/score - 1)
    constexpr double eps = 1e-12;
    const double p = std::clamp(score, eps, 1.0 - eps);
    return -400.0 * std::log10(1.0 / p - 1.0);
}

double score_to_nelo_diff(double score, double variance) {
    // Match fastchess EloWDL convention:
    // (score - 0.5) / sqrt(variance) * (800 / ln(10))
    constexpr double min_var = 1e-30;
    const double v = std::max(variance, min_var);
    return (score - 0.5) / std::sqrt(v) * (800.0 / std::log(10.0));
}

} // namespace

std::optional<FastchessHeadToHeadStats> compute_fastchess_head_to_head(
    const std::vector<Pairing>& pairings,
    const std::vector<std::string>& names,
    std::size_t a_index,
    std::size_t b_index) {
    if (a_index == b_index || a_index >= names.size() || b_index >= names.size()) {
        return std::nullopt;
    }

    FastchessHeadToHeadStats out;
    out.player_a = names[a_index];
    out.player_b = names[b_index];

    for (const auto& p : pairings) {
        const bool a_as_white = (p.white == a_index && p.black == b_index);
        const bool a_as_black = (p.white == b_index && p.black == a_index);
        if (!a_as_white && !a_as_black) {
            // Require a strict 1v1 dataset: anything else is ambiguous for fastchess-style stats.
            return std::nullopt;
        }

        out.games += 1;
        double a_score = a_as_white ? p.score : (1.0 - p.score);

        if (a_score == 1.0) {
            out.wins += 1;
        } else if (a_score == 0.0) {
            out.losses += 1;
        } else {
            out.draws += 1;
        }
    }

    if (out.games == 0) {
        return out;
    }

    out.score = (static_cast<double>(out.wins) + 0.5 * static_cast<double>(out.draws)) / static_cast<double>(out.games);
    out.score_pct = out.score * 100.0;
    out.draw_pct = (static_cast<double>(out.draws) / static_cast<double>(out.games)) * 100.0;

    // Match fastchess EloWDL variance model.
    const double W = static_cast<double>(out.wins) / static_cast<double>(out.games);
    const double D = static_cast<double>(out.draws) / static_cast<double>(out.games);
    const double L = static_cast<double>(out.losses) / static_cast<double>(out.games);

    const double score = out.score;
    const double W_dev = W * std::pow((1.0 - score), 2);
    const double D_dev = D * std::pow((0.5 - score), 2);
    const double L_dev = L * std::pow((0.0 - score), 2);
    const double variance = W_dev + D_dev + L_dev;
    const double variance_per_game = variance / static_cast<double>(out.games);

    // 95% CI on score then map through Elo transform (fastchess convention).
    constexpr double z95 = 1.959963984540054;
    const double score_upper = score + z95 * std::sqrt(variance_per_game);
    const double score_lower = score - z95 * std::sqrt(variance_per_game);

    out.elo = score_to_elo_diff(score);
    out.elo_error_95 = (score_to_elo_diff(score_upper) - score_to_elo_diff(score_lower)) / 2.0;

    out.nelo = score_to_nelo_diff(score, variance);
    out.nelo_error_95 = (score_to_nelo_diff(score_upper, variance) - score_to_nelo_diff(score_lower, variance)) / 2.0;

    // LOS computed in score-space (fastchess convention).
    if (variance_per_game <= 0.0) {
        out.los = score > 0.5 ? 1.0 : (score < 0.5 ? 0.0 : 0.5);
    } else {
        out.los = (1.0 - std::erf(-(score - 0.5) / std::sqrt(2.0 * variance_per_game))) / 2.0;
    }

    return out;
}

} // namespace bayeselo
