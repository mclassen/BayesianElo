#include <cassert>
#include <string>
#include <vector>

#include "filters/game_filter.h"

using namespace bayeselo;

bool test_filters() {
    Game g;
    g.tags.white = "Alpha";
    g.tags.black = "Beta";
    g.tags.result = "1/2-1/2";
    g.ply_count = 60;
    g.duration_seconds = 600;

    FilterOptions options;
    options.min_plies = 40;
    options.max_plies = 80;
    options.min_time = 300u;
    options.either_name = "Alpha";
    options.result = "draw";

    GameFilter filter(options);
    assert(filter(g));

    options.max_plies = 50;
    GameFilter filter2(options);
    assert(!filter2(g));
    return true;
}
