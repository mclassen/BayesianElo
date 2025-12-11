#include <cassert>
#include <vector>

#include "rating/bayeselo_solver.h"

using namespace bayeselo;

bool test_rating() {
    Game g1{{"Alpha", "Beta", "1-0", {}, {}, {}, {}}, {}, 0, {}};
    Game g2{{"Beta", "Gamma", "1-0", {}, {}, {}, {}}, {}, 0, {}};
    Game g3{{"Gamma", "Alpha", "0-1", {}, {}, {}, {}}, {}, 0, {}};
    std::vector<Game> games{g1, g2, g3};

    BayesEloSolver solver;
    auto res = solver.solve(games);
    assert(!res.players.empty());
    return true;
}
