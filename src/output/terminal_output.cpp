#include "terminal_output.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#ifdef _WIN32
#include <io.h>
#ifndef isatty
#define isatty _isatty
#endif
#ifndef fileno
#define fileno _fileno
#endif
#else
#include <unistd.h>
#endif

namespace bayeselo {

namespace {
bool colors_enabled() {
    if (std::getenv("NO_COLOR")) return false;
    return isatty(fileno(stdout)) != 0;
}

std::string colorize(double rating, std::size_t rank) {
    if (!colors_enabled()) return {};
    if (rank == 0) return "\033[1;32m"; // top
    if (rating < 0) return "\033[1;33m";
    return "\033[0m";
}
}

void print_ratings(const RatingResult& result) {
    // Players are already sorted by rating in BayesEloSolver.
    std::cout << "Rank | Player | Elo | Error | Games | Score% | Draw%\n";
    std::cout << "-----------------------------------------------------\n";
    auto old_flags = std::cout.flags();
    auto old_precision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t i = 0; i < result.players.size(); ++i) {
        const auto& p = result.players[i];
        double score_pct = p.games_played ? (p.score_sum / p.games_played) * 100.0 : 0.0;
        double draw_pct = p.games_played ? (static_cast<double>(p.draws) / p.games_played) * 100.0 : 0.0;
        auto color = colorize(p.rating, i);
        std::cout << color
                  << std::right << std::setw(4) << (i + 1) << " | "
                  << std::left << std::setw(20) << p.name << " | "
                  << std::right << std::setw(7) << p.rating << " | "
                  << std::setw(6) << p.error << " | "
                  << std::setw(5) << p.games_played << " | "
                  << std::setw(6) << score_pct << "% | "
                  << std::setw(6) << draw_pct << "%\033[0m\n";
    }
    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

} // namespace bayeselo
