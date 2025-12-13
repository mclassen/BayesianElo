#include "terminal_output.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
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
std::size_t total_games_in_result(const RatingResult& result) {
    std::size_t sum = 0;
    for (const auto& p : result.players) {
        sum += p.games_played;
    }
    return sum / 2;
}

void print_total_games_line(const RatingResult& result, std::size_t planned_games) {
    const std::size_t total_games = total_games_in_result(result);
    std::cout << "Games: " << total_games;
    if (planned_games != 0) {
        std::cout << " / " << planned_games;
    }
    std::cout << "\n";
}

bool colors_enabled() {
    if (std::getenv("NO_COLOR")) return false;
    return isatty(fileno(stdout)) != 0;
}

std::string colorize(double rating, std::size_t rank) {
    if (!colors_enabled()) {
        return {};
    }
    if (rank == 0) {
        return "\033[1;32m"; // top
    }
    if (rating < 0) {
        return "\033[1;33m";
    }
    return {};
}
}

void print_ratings(const RatingResult& result, std::size_t planned_games) {
    // Players are already sorted by rating in BayesEloSolver.
    print_total_games_line(result, planned_games);
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
                  << std::setw(6) << draw_pct << "%";
        if (!color.empty()) {
            std::cout << "\033[0m";
        }
        std::cout << "\n";
    }
    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

void print_los_matrix(const RatingResult& result) {
    const auto& los = result.los_matrix;
    const std::size_t n = result.players.size();
    if (n == 0 || los.size() != n) {
        return;
    }

    // Basic sanity: ensure each row has n entries.
    for (const auto& row : los) {
        if (row.size() != n) {
            return;
        }
    }

    auto old_flags = std::cout.flags();
    auto old_precision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(1);

    const int name_width = 14;
    const int cell_width = 8;
    auto abbrev = [&](const std::string& name) {
        if (static_cast<int>(name.size()) <= name_width - 1) {
            return name;
        }
        return name.substr(0, name_width - 2) + "…";
    };

    std::cout << "\nLOS matrix (row is probability to beat column, %)\n";
    std::cout << std::setw(name_width) << "";
    for (std::size_t j = 0; j < n; ++j) {
        std::cout << std::setw(cell_width) << abbrev(result.players[j].name);
    }
    std::cout << "\n";

    for (std::size_t i = 0; i < n; ++i) {
        std::cout << std::setw(name_width) << abbrev(result.players[i].name);
        for (std::size_t j = 0; j < n; ++j) {
            if (i == j) {
                std::cout << std::setw(cell_width) << "--";
            } else {
                std::cout << std::setw(cell_width) << (los[i][j] * 100.0);
            }
        }
        std::cout << "\n";
    }

    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

void print_ratings_markdown(const RatingResult& result, std::size_t planned_games) {
    print_total_games_line(result, planned_games);
    std::cout << "\n";
    std::cout << "| Rank | Player | Elo | Error | Games | Score% | Draw% |\n";
    std::cout << "| ---: | :----- | ---: | ----: | ----: | -----: | ----: |\n";
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t i = 0; i < result.players.size(); ++i) {
        const auto& p = result.players[i];
        const double score_pct = p.games_played ? (p.score_sum / p.games_played) * 100.0 : 0.0;
        const double draw_pct = p.games_played ? (static_cast<double>(p.draws) / p.games_played) * 100.0 : 0.0;
        std::cout << "| " << (i + 1)
                  << " | " << p.name
                  << " | " << p.rating
                  << " | " << p.error
                  << " | " << p.games_played
                  << " | " << score_pct
                  << " | " << draw_pct
                  << " |\n";
    }
}

void print_los_matrix_markdown(const RatingResult& result) {
    const auto& los = result.los_matrix;
    const std::size_t n = result.players.size();
    if (n == 0 || los.size() != n) {
        return;
    }
    for (const auto& row : los) {
        if (row.size() != n) {
            return;
        }
    }

    std::cout << "\n| LOS% |";
    for (std::size_t j = 0; j < n; ++j) {
        std::cout << " " << result.players[j].name << " |";
    }
    std::cout << "\n| :--- |";
    for (std::size_t j = 0; j < n; ++j) {
        (void)j;
        std::cout << " ---: |";
    }
    std::cout << "\n";

    std::cout << std::fixed << std::setprecision(1);
    for (std::size_t i = 0; i < n; ++i) {
        std::cout << "| " << result.players[i].name << " |";
        for (std::size_t j = 0; j < n; ++j) {
            if (i == j) {
                std::cout << " -- |";
            } else {
                std::cout << " " << (los[i][j] * 100.0) << " |";
            }
        }
        std::cout << "\n";
    }
}

} // namespace bayeselo
