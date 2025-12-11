#include "rating/bayeselo_solver.h"
#include "bayeselo/game.h"

#include <cassert>
#include <vector>
#include <iostream>

int main() {
    using bayeselo::BayesEloSolver;
    using bayeselo::Game;
    using bayeselo::GameResult;

    Game g1{{"Alpha", "Beta", {}, {}, {}}, {GameResult::Outcome::WhiteWin, {}}, {}, 0, {}};
    Game g2{{"Alpha", "Gamma", {}, {}, {}}, {GameResult::Outcome::WhiteWin, {}}, {}, 0, {}};
    Game g3{{"Beta", "Gamma", {}, {}, {}}, {GameResult::Outcome::WhiteWin, {}}, {}, 0, {}};

    std::vector<Game> games{g1, g2, g3};
    BayesEloSolver solver;
    auto res = solver.solve(games);

    assert(res.players.size() == 3);
    // Ratings should be sorted descending
    assert(res.players[0].rating > res.players[1].rating);
    assert(res.players[1].rating > res.players[2].rating);
    // LOS matrix should align with sorted order
    assert(res.los_matrix.size() == res.players.size());
    assert(res.los_matrix[0].size() == res.players.size());
    assert(res.los_matrix[0][1] > 0.5); // Alpha favored over Beta
    assert(res.los_matrix[1][2] > 0.5); // Beta favored over Gamma

    // Pairings-based path should match
    std::vector<bayeselo::Pairing> pairings;
    pairings.push_back({0, 1, 1.0});
    pairings.push_back({0, 2, 1.0});
    pairings.push_back({1, 2, 1.0});
    std::vector<std::string> names{"Alpha", "Beta", "Gamma"};
    auto res2 = solver.solve(pairings, names);
    assert(res2.players.size() == 3);
    assert(res2.players[0].name == "Alpha");
    assert(res2.los_matrix[0][1] > 0.5);

    std::cout << "rating tests passed\n";
    return 0;
}
