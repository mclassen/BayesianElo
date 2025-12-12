#pragma once

#include <chrono>
#include <string_view>

namespace bayeselo {

double parse_duration_to_seconds(std::string_view value);

} // namespace bayeselo
