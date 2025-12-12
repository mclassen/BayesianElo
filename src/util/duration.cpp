#include "bayeselo/duration.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace bayeselo {

double parse_duration_to_seconds(std::string_view value) {
    if (value.empty()) {
        throw std::invalid_argument("Invalid duration: (empty)");
    }
    std::string numeric;
    char suffix = 's';
    for (char ch : value) {
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.') {
            numeric.push_back(ch);
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(ch))) {
            char lower_ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (lower_ch == 'h' || lower_ch == 'm' || lower_ch == 's') {
                suffix = lower_ch;
            } else {
                throw std::invalid_argument("Invalid duration suffix: " + std::string(1, ch) + " in " + std::string(value));
            }
        }
        break; // stop at the first non-numeric marker (e.g. '+' in "300+2")
    }
    if (numeric.empty()) {
        throw std::invalid_argument("Invalid duration: " + std::string(value));
    }
    errno = 0;
    char* endptr = nullptr;
    double number = std::strtod(numeric.c_str(), &endptr);
    if (errno == ERANGE || endptr == numeric.c_str() || *endptr != '\0') {
        throw std::invalid_argument("Invalid duration: " + std::string(value));
    }
    switch (suffix) {
    case 'h':
        return number * 3600.0;
    case 'm':
        return number * 60.0;
    case 's':
        return number;
    default:
        throw std::invalid_argument("Invalid duration suffix: " + std::string(1, suffix) + " in " + std::string(value));
    }
}

} // namespace bayeselo
