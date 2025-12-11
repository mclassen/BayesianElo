#include "export_writer.h"

#include <format>
#include <fstream>
#include <stdexcept>

namespace bayeselo {

void write_csv(const RatingResult& result, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to open CSV output: " + path.string());
    }
    out << "Player,Elo,Error,Games,ScorePct,DrawPct\n";
    for (const auto& p : result.players) {
        double score_pct = p.games_played ? (p.score_sum / p.games_played) * 100.0 : 0.0;
        double draw_pct = p.games_played ? (p.draws / p.games_played) * 100.0 : 0.0;
        out << std::format("{},{:.2f},{:.2f},{},{:.2f},{:.2f}\n", p.name, p.rating, p.error, p.games_played, score_pct, draw_pct);
    }
}

void write_json(const RatingResult& result, const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to open JSON output: " + path.string());
    }
    out << "{\n  \"players\": [\n";
    for (std::size_t i = 0; i < result.players.size(); ++i) {
        const auto& p = result.players[i];
        double score_pct = p.games_played ? (p.score_sum / p.games_played) * 100.0 : 0.0;
        double draw_pct = p.games_played ? (p.draws / p.games_played) * 100.0 : 0.0;
        out << std::format("    {{\"name\": \"{}\", \"elo\": {:.2f}, \"error\": {:.2f}, \"games\": {}, \"score_pct\": {:.2f}, \"draw_pct\": {:.2f}}}",
                          p.name, p.rating, p.error, p.games_played, score_pct, draw_pct);
        if (i + 1 != result.players.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n  \"los\": [\n";
    for (std::size_t i = 0; i < result.los_matrix.size(); ++i) {
        out << "    [";
        for (std::size_t j = 0; j < result.los_matrix[i].size(); ++j) {
            out << std::format("{:.4f}", result.los_matrix[i][j]);
            if (j + 1 != result.los_matrix[i].size()) out << ", ";
        }
        out << "]";
        if (i + 1 != result.los_matrix.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}";
}

} // namespace bayeselo
