#include "rating/bayeselo_solver.h"
#include "bayeselo/game.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

int main() {
    using bayeselo::BayesEloSolver;
    using bayeselo::Game;
    using bayeselo::GameResult;
    auto fail = [](const std::string& msg) {
        std::cerr << msg << "\n";
        return 1;
    };

    Game g1{{"Alpha", "Beta", {}, {}, {}}, {GameResult::Outcome::WhiteWin, {}}, {}, 0, {}};
    Game g2{{"Alpha", "Gamma", {}, {}, {}}, {GameResult::Outcome::WhiteWin, {}}, {}, 0, {}};
    Game g3{{"Beta", "Gamma", {}, {}, {}}, {GameResult::Outcome::WhiteWin, {}}, {}, 0, {}};

    std::vector<Game> games{g1, g2, g3};
    BayesEloSolver solver;
    auto res = solver.solve(games);

    if (res.players.size() != 3) return fail("expected 3 players, got " + std::to_string(res.players.size()));
    // Ratings should be sorted descending
    if (!(res.players[0].rating > res.players[1].rating)) return fail("players not sorted desc (0 vs 1)");
    if (!(res.players[1].rating > res.players[2].rating)) return fail("players not sorted desc (1 vs 2)");
    // LOS matrix should align with sorted order
    if (res.los_matrix.size() != res.players.size()) return fail("los_matrix row count mismatch");
    if (res.los_matrix[0].size() != res.players.size()) return fail("los_matrix col count mismatch");
    if (!(res.los_matrix[0][1] > 0.5)) return fail("expected LOS Alpha>Beta");
    if (!(res.los_matrix[1][2] > 0.5)) return fail("expected LOS Beta>Gamma");

    // Pairings-based path should match
    std::vector<bayeselo::Pairing> pairings;
    pairings.push_back({0, 1, 1.0});
    pairings.push_back({0, 2, 1.0});
    pairings.push_back({1, 2, 1.0});
    std::vector<std::string> names{"Alpha", "Beta", "Gamma"};
    auto res2 = solver.solve(pairings, names);
    if (res2.players.size() != 3) return fail("pairings solve expected 3 players");
    if (res2.players[0].name != "Alpha") return fail("pairings solve top player not Alpha");
    if (!(res2.los_matrix[0][1] > 0.5)) return fail("pairings solve expected LOS Alpha>Beta");

    // Anchor player should stay at anchor rating.
    const double anchor_rating = 123.45;
    auto anchored = solver.solve(games, std::optional<std::string>{"Alpha"}, anchor_rating);
    auto it = std::find_if(anchored.players.begin(), anchored.players.end(),
                           [](const auto& p) { return p.name == "Alpha"; });
    if (it == anchored.players.end()) return fail("anchored solve missing Alpha");
    if (std::abs(it->rating - anchor_rating) > 1e-9) return fail("Alpha not anchored to rating");
    if (anchored.players.size() != res.players.size()) return fail("anchored solve player count mismatch");
    if (anchored.players[0].name != res.players[0].name) return fail("anchored ordering changed unexpectedly");

    // Non-existent anchor should behave like no anchor.
    auto missing_anchor = solver.solve(games, std::optional<std::string>{"NoSuchPlayer"}, 999.0);
    if (missing_anchor.players.size() != res.players.size()) return fail("missing anchor changed player count");
    if (missing_anchor.players[0].name != res.players[0].name) return fail("missing anchor changed ordering");

    std::cout << "rating tests passed\n";
    return 0;
}
