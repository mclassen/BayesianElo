#pragma once

#include "bayeselo/game.h"
#include "rating/bayeselo_solver.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace bayeselo {

// "Fastchess-style" 1v1 Elo from score, with error derived via the delta method on trinomial outcomes
// and LOS derived from that error (matching fastchess's log output).
struct FastchessHeadToHeadStats {
    std::string player_a;
    std::string player_b;
    std::size_t games{0};
    std::size_t wins{0};
    std::size_t losses{0};
    std::size_t draws{0};
    double score{0.0};        // points / games, from A's perspective
    double score_pct{0.0};    // score * 100
    double draw_pct{0.0};     // draws / games * 100
    double elo{0.0};          // Elo(A-B)
    double elo_error_95{0.0}; // 95% CI half-width (fastchess "+/-")
    double nelo{0.0};         // normalized Elo (fastchess "nElo")
    double nelo_error_95{0.0};
    double los{0.5};          // P(Elo>0), from A's perspective
};

// Computes head-to-head stats for names[a_index] vs names[b_index]. Returns nullopt if any pairing
// involves a player outside that set.
std::optional<FastchessHeadToHeadStats> compute_fastchess_head_to_head(
    const std::vector<Pairing>& pairings,
    const std::vector<std::string>& names,
    std::size_t a_index = 0,
    std::size_t b_index = 1);

} // namespace bayeselo
