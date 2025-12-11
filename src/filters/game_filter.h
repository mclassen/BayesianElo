#pragma once

#include <functional>
#include <optional>
#include <string>

#include "../../include/bayeselo/game.h"

namespace bayeselo {

struct FilterOptions {
    std::optional<std::uint32_t> min_plies;
    std::optional<std::uint32_t> max_plies;
    std::optional<std::uint32_t> min_time;
    std::optional<std::uint32_t> max_time;
    std::optional<std::string> white_name;
    std::optional<std::string> black_name;
    std::optional<std::string> either_name;
    std::optional<std::string> exclude_name;
    std::optional<std::string> result;
    std::optional<std::string> termination;
    bool require_complete{false};
    bool skip_empty{false};
};

class GameFilter {
public:
    GameFilter() = default;
    explicit GameFilter(FilterOptions options);
    bool operator()(const Game& game) const;

private:
    static bool match_name(const std::string& player, const std::optional<std::string>& query);
    FilterOptions options_{};
};

}  // namespace bayeselo
