#include "bayeselo/size_parse.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <limits>
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
    if (!std::all_of(number_part.begin(), number_part.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        return std::nullopt;
    }
    try {
        const unsigned long long number = std::stoull(number_part);
        if (number > std::numeric_limits<std::size_t>::max() / multiplier) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(number) * multiplier;
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
