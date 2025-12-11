#include "bayeselo/duration.h"

#include <charconv>
#include <cctype>
#include <stdexcept>
#include <string>

namespace bayeselo {

double parse_duration_to_seconds(std::string_view value) {
    if (value.empty()) {
        return 0.0;
    }
    std::string numeric;
    char suffix = 's';
    for (char ch : value) {
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.') {
            numeric.push_back(ch);
        } else {
            suffix = ch;
        }
    }
    double number = 0.0;
    auto res = std::from_chars(numeric.data(), numeric.data() + numeric.size(), number);
    if (res.ec != std::errc{}) {
        throw std::invalid_argument("Invalid duration: " + std::string(value));
    }
    switch (suffix) {
    case 'h':
    case 'H':
        return number * 3600.0;
    case 'm':
    case 'M':
        return number * 60.0;
    case 's':
    case 'S':
    default:
        return number;
    }
}

} // namespace bayeselo

