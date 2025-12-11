#include "export_writer.h"

#include <fstream>
#include <format>

namespace bayeselo {

void ExportWriter::write_csv(const std::string& path, const RatingResult& result) const {
    std::ofstream out(path);
    out << "rank,player,elo,error,games\n";
    int rank = 1;
    for (const auto& p : result.players) {
        out << std::format("{},{},{:.2f},{:.2f},{}\n", rank, p.name, p.rating, p.error, p.games);
        ++rank;
    }
}

void ExportWriter::write_json(const std::string& path, const RatingResult& result) const {
    std::ofstream out(path);
    out << "{\n  \"players\": [\n";
    for (std::size_t i = 0; i < result.players.size(); ++i) {
        const auto& p = result.players[i];
        out << std::format("    {{\n      \"name\": \"{}\",\n      \"elo\": {:.2f},\n      \"error\": {:.2f},\n      \"games\": {}\n    }}{}\n",
                            p.name, p.rating, p.error, p.games, (i + 1 == result.players.size() ? "" : ","));
    }
    out << "  ],\n  \"los\": [\n";
    for (std::size_t i = 0; i < result.los_matrix.size(); ++i) {
        out << "    [";
        for (std::size_t j = 0; j < result.los_matrix[i].size(); ++j) {
            out << std::format("{:.4f}{}", result.los_matrix[i][j], (j + 1 == result.los_matrix[i].size() ? "" : ", "));
        }
        out << "]" << (i + 1 == result.los_matrix.size() ? "\n" : ",\n");
    }
    out << "  ]\n}\n";
}

}  // namespace bayeselo
