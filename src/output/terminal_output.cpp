#include "terminal_output.h"

#include <cmath>
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

double normal_cdf(double z) {
    // Φ(z) = 0.5 * erfc(-z / sqrt(2))
    constexpr double inv_sqrt2 = 0.70710678118654752440;
    return 0.5 * std::erfc(-z * inv_sqrt2);
}

double los_p_gt_0(double rating_diff, double error_a, double error_b) {
    const double sd = std::sqrt(error_a * error_a + error_b * error_b);
    if (sd <= 0.0) {
        return rating_diff > 0.0 ? 1.0 : (rating_diff < 0.0 ? 0.0 : 0.5);
    }
    return normal_cdf(rating_diff / sd);
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

    std::cout << "\nLOS matrix (P(Elo_row > Elo_col), %)\n";
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
                const double diff = result.players[i].rating - result.players[j].rating;
                const double p = los_p_gt_0(diff, result.players[i].error, result.players[j].error);
                std::cout << std::setw(cell_width) << (p * 100.0);
            }
        }
        std::cout << "\n";
    }

    std::cout << "\nEloLogit matrix (10^(-diff/200) logistic, %, BayesElo-style)\n";
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

    std::cout << "\n| LOS% (P(Elo_row > Elo_col)) |";
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
                const double diff = result.players[i].rating - result.players[j].rating;
                const double p = los_p_gt_0(diff, result.players[i].error, result.players[j].error);
                std::cout << " " << (p * 100.0) << " |";
            }
        }
        std::cout << "\n";
    }

    std::cout << "\n| EloLogit% (10^(-diff/200) logistic) |";
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

void print_fastchess_head_to_head(const FastchessHeadToHeadStats& stats, std::size_t planned_games) {
    std::cout << "Games: " << stats.games;
    if (planned_games != 0) {
        std::cout << " / " << planned_games;
    }
    std::cout << "\n";

    std::cout << "Players: " << stats.player_a << " vs " << stats.player_b << "\n";
    std::cout << "W-D-L : " << stats.wins << "-" << stats.draws << "-" << stats.losses << "\n";

    auto old_flags = std::cout.flags();
    auto old_precision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Score : " << stats.score_pct << "%  (Draw " << stats.draw_pct << "%)\n";
    std::cout << "Elo   : " << stats.elo << " +/- " << stats.elo_error_95 << " (95% CI)\n";
    std::cout << "nElo  : " << stats.nelo << " +/- " << stats.nelo_error_95 << " (95% CI)\n";
    std::cout << std::setprecision(2) << "LOS   : " << (stats.los * 100.0) << "%\n";
    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

void print_fastchess_head_to_head_markdown(const FastchessHeadToHeadStats& stats, std::size_t planned_games) {
    std::cout << "Games: " << stats.games;
    if (planned_games != 0) {
        std::cout << " / " << planned_games;
    }
    std::cout << "\n\n";

    std::cout << "| Player A | Player B | Elo(A-B) | +/- (95%) | nElo | +/- (95%) | LOS% | Games | W | D | L | Score% | Draw% |\n";
    std::cout << "| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n";

    auto old_flags = std::cout.flags();
    auto old_precision = std::cout.precision();
    std::cout << std::fixed;
    std::cout << std::setprecision(2);
    std::cout << "| " << stats.player_a
              << " | " << stats.player_b
              << " | " << stats.elo
              << " | " << stats.elo_error_95
              << " | " << stats.nelo
              << " | " << stats.nelo_error_95
              << " | " << (stats.los * 100.0)
              << " | " << stats.games
              << " | " << stats.wins
              << " | " << stats.draws
              << " | " << stats.losses
              << " | " << stats.score_pct
              << " | " << stats.draw_pct
              << " |\n";

    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

} // namespace bayeselo
