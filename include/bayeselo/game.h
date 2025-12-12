#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <optional>

namespace bayeselo {

struct GameResult {
    enum class Outcome {
        WhiteWin,
        BlackWin,
        Draw,
        Unknown
    } outcome{Outcome::Unknown};
    std::optional<std::string> termination;
};

struct GameMetadata {
    std::string white;
    std::string black;
    std::optional<std::string> utc_date;
    std::optional<std::string> utc_time;
    std::optional<std::string> time_control;
};

struct Game {
    GameMetadata meta;
    GameResult result;
    std::vector<std::string> moves;
    std::uint32_t ply_count{0};
    std::optional<double> estimated_duration_seconds;
};

struct PlayerStats {
    std::string name;
    double rating{0.0};
    double error{0.0};
    std::uint32_t games_played{0};
    double score_sum{0.0};
    double opponent_rating_sum{0.0};
    std::uint32_t draws{0};
};

} // namespace bayeselo
