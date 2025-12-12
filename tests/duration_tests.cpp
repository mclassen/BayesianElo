#include "bayeselo/duration.h"

#include <iostream>

int main() {
    using bayeselo::parse_duration_to_seconds;
    auto check = [&](const char* text, double expected) -> bool {
        double got = parse_duration_to_seconds(text);
        if (got != expected) {
            std::cerr << "duration test failed: \"" << text << "\" expected " << expected << " got " << got << "\n";
            return false;
        }
        return true;
    };
    if (!check("60", 60.0)) return 1;
    if (!check("2m", 120.0)) return 1;
    if (!check("1h", 3600.0)) return 1;
    if (!check("300+2", 300.0)) return 1;
    if (!check("5m+3", 300.0)) return 1;
    std::cout << "duration tests passed\n";
    return 0;
}
