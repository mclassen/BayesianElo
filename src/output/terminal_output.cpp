#include "terminal_output.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <ranges>

namespace bayeselo {

namespace {
std::string colorize(double rating, std::size_t rank) {
    if (rank == 0) return "\033[1;32m"; // top
    if (rating < 0) return "\033[1;33m";
    return "\033[0m";
}
}

void print_ratings(const RatingResult& result) {
    std::vector<PlayerStats> sorted = result.players;
    std::ranges::sort(sorted, [](const PlayerStats& a, const PlayerStats& b) { return a.rating > b.rating; });
    std::cout << "Rank | Player | Elo | Error | Games | Score% | Draw%\n";
    std::cout << "-----------------------------------------------------\n";
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const auto& p = sorted[i];
        double score_pct = p.games_played ? (p.score_sum / p.games_played) * 100.0 : 0.0;
        double draw_pct = p.games_played ? (p.draws / p.games_played) * 100.0 : 0.0;
        auto color = colorize(p.rating, i);
        std::cout << std::format("{}{:>4} | {:<20} | {:7.2f} | {:6.2f} | {:5} | {:6.2f}% | {:6.2f}%\033[0m\n",
                                 color, i + 1, p.name, p.rating, p.error, p.games_played, score_pct, draw_pct);
    }
}

} // namespace bayeselo
