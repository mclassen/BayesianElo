#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

namespace bayeselo {

// Parse byte-size strings with optional k/m/g suffixes (KiB, MiB, GiB).
std::optional<std::size_t> parse_size(std::string_view text);

// Helper that falls back to a default when parsing fails.
std::size_t parse_size_or(std::string_view text, std::size_t fallback);

} // namespace bayeselo
