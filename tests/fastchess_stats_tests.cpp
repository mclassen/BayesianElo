#include "bayeselo/fastchess_stats.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using bayeselo::compute_fastchess_head_to_head;
    using bayeselo::Pairing;

    auto fail = [](const std::string& msg) {
        std::cerr << msg << "\n";
        return 1;
    };

    // Build a small 1v1 set with A vs B:
    // W-D-L = 4-3-3 (10 games) from A's perspective.
    std::vector<std::string> names{"A", "B"};
    std::vector<Pairing> pairings;
    pairings.reserve(10);
    // A as white wins 2, draws 1, loses 1
    pairings.push_back({0, 1, 1.0});
    pairings.push_back({0, 1, 1.0});
    pairings.push_back({0, 1, 0.5});
    pairings.push_back({0, 1, 0.0});
    // A as black wins 2, draws 2, loses 2 (white is B, score is from white)
    pairings.push_back({1, 0, 0.0}); // A wins
    pairings.push_back({1, 0, 0.0}); // A wins
    pairings.push_back({1, 0, 0.5}); // draw
    pairings.push_back({1, 0, 0.5}); // draw
    pairings.push_back({1, 0, 1.0}); // A loses
    pairings.push_back({1, 0, 1.0}); // A loses

    auto stats = compute_fastchess_head_to_head(pairings, names, 0, 1);
    if (!stats) return fail("expected stats for strict 1v1");

    if (stats->games != 10) return fail("games mismatch");
    if (stats->wins != 4) return fail("wins mismatch");
    if (stats->draws != 3) return fail("draws mismatch");
    if (stats->losses != 3) return fail("losses mismatch");

    const double expected_score = (4.0 + 0.5 * 3.0) / 10.0;
    if (std::abs(stats->score - expected_score) > 1e-12) return fail("score mismatch");

    const double expected_elo = -400.0 * std::log10(1.0 / expected_score - 1.0);
    if (std::abs(stats->elo - expected_elo) > 1e-9) return fail("elo mismatch");

    // Sanity: errors should be positive and LOS should correspond to score/Elo sign.
    if (!(stats->elo_error_95 > 0.0)) return fail("expected positive error");
    if (stats->elo > 0.0 && !(stats->los > 0.5)) return fail("expected LOS > 0.5 for positive Elo");
    if (stats->elo < 0.0 && !(stats->los < 0.5)) return fail("expected LOS < 0.5 for negative Elo");
    if (!(stats->nelo_error_95 > 0.0)) return fail("expected positive nelo error");

    // Non-1v1 datasets must fail.
    std::vector<std::string> names3{"A", "B", "C"};
    std::vector<Pairing> pairings3 = pairings;
    pairings3.push_back({2, 0, 1.0});
    if (compute_fastchess_head_to_head(pairings3, names3, 0, 1)) {
        return fail("expected nullopt when pairings include third player");
    }

    std::cout << "fastchess stats tests passed\n";
    return 0;
}
