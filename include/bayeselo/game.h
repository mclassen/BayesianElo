#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace bayeselo {

struct GameTags {
    std::string white;
    std::string black;
    std::string result;
    std::optional<std::string> termination;
    std::optional<std::string> utc_date;
    std::optional<std::string> utc_time;
    std::optional<std::string> time_control;
};

struct Game {
    GameTags tags;
    std::vector<std::string> moves;
    std::uint32_t ply_count{};
    std::optional<std::uint32_t> duration_seconds;
};

struct PlayerStats {
    std::string name;
    double rating{0.0};
    double error{0.0};
    std::size_t games{0};
    double score{0.0};
    double draw_rate{0.0};
    double opponent_avg{0.0};
};

struct RatingResult {
    std::vector<PlayerStats> players;
    std::vector<std::vector<double>> los_matrix;
};

}  // namespace bayeselo
