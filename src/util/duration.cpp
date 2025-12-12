#include "bayeselo/duration.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
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
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(ch))) {
            suffix = ch;
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
