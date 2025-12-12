#include "bayeselo/size_parse.h"

#include <exception>
#include <optional>
#include <string>

namespace bayeselo {

std::optional<std::size_t> parse_size(std::string_view text) {
    if (text.empty()) return std::nullopt;
    std::size_t multiplier = 1;
    char suffix = text.back();
    if (suffix == 'k' || suffix == 'K') multiplier = 1024;
    else if (suffix == 'm' || suffix == 'M') multiplier = 1024ull * 1024ull;
    else if (suffix == 'g' || suffix == 'G') multiplier = 1024ull * 1024ull * 1024ull;
    else suffix = '\0';

    std::string number_part = (suffix == '\0') ? std::string(text) : std::string(text.substr(0, text.size() - 1));
    if (number_part.empty()) return std::nullopt;
    if (number_part.front() == '-') return std::nullopt;
    try {
        return static_cast<std::size_t>(std::stoull(number_part)) * multiplier;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::size_t parse_size_or(std::string_view text, std::size_t fallback) {
    auto parsed = parse_size(text);
    if (!parsed) return fallback;
    return *parsed;
}

} // namespace bayeselo
