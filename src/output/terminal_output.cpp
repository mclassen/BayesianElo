#include "terminal_output.h"

#include <format>
#include <iostream>

namespace bayeselo {

void TerminalOutput::print_summary(const RatingResult& result) const {
    std::cout << "\033[1;32mElo Summary\033[0m\n";
    std::cout << std::format("{:<4} {:<20} {:>8} {:>8} {:>8}\n", "Rank", "Player", "Elo", "+/-", "Games");
    int rank = 1;
    for (const auto& p : result.players) {
        std::string color = rank == 1 ? "\033[1;33m" : "\033[0m";
        std::cout << color
                  << std::format("{:<4} {:<20} {:>8.1f} {:>8.1f} {:>8}\n", rank, p.name, p.rating, p.error, p.games)
                  << "\033[0m";
        ++rank;
    }
}

void TerminalOutput::print_los(const RatingResult& result) const {
    if (result.players.size() > 8) return;
    std::cout << "\n\033[1;34mLOS Matrix\033[0m\n";
    std::cout << "      ";
    for (const auto& p : result.players) {
        std::cout << std::format("{:>8}", p.name.substr(0, 6));
    }
    std::cout << "\n";
    for (std::size_t i = 0; i < result.players.size(); ++i) {
        std::cout << std::format("{:<6}", result.players[i].name.substr(0, 6));
        for (std::size_t j = 0; j < result.players.size(); ++j) {
            std::cout << std::format("{:>8.1f}", result.los_matrix[i][j] * 100.0);
        }
        std::cout << "\n";
    }
}

}  // namespace bayeselo
