#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace bayeselo {

struct FilterConfig {
    std::optional<std::uint32_t> min_plies;
    std::optional<std::uint32_t> max_plies;
    std::optional<double> min_time_seconds;
    std::optional<double> max_time_seconds;
    std::optional<std::string> white_name;
    std::optional<std::string> black_name;
    std::optional<std::string> either_name;
    std::optional<std::string> exclude_name;
    std::optional<std::string> result_filter;
    std::optional<std::string> termination;
    bool require_complete{false};
    bool skip_empty{false};
};

} // namespace bayeselo

