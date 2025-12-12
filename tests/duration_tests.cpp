#include "bayeselo/duration.h"

#include <cmath>
#include <iostream>

int main() {
    using bayeselo::parse_duration_to_seconds;
    auto check = [](const char* text, double expected) -> bool {
        double got = parse_duration_to_seconds(text);
        if (std::abs(got - expected) > 1e-9) {
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
    try {
        parse_duration_to_seconds("abc");
        std::cerr << "duration test failed: \"abc\" should throw\n";
        return 1;
    } catch (const std::exception&) {
    }
    try {
        parse_duration_to_seconds("5x+3");
        std::cerr << "duration test failed: \"5x+3\" should throw\n";
        return 1;
    } catch (const std::exception&) {
    }
    if (!check("", 0.0)) return 1; // empty input treated as zero per parser contract
    try {
        parse_duration_to_seconds("-5m");
        std::cerr << "duration test failed: \"-5m\" should throw\n";
        return 1;
    } catch (const std::exception&) {
    }
    std::cout << "duration tests passed\n";
    return 0;
}
