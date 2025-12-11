#include "bayeselo/duration.h"

#include <cassert>
#include <iostream>

int main() {
    using bayeselo::parse_duration_to_seconds;
    assert(parse_duration_to_seconds("60") == 60.0);
    assert(parse_duration_to_seconds("2m") == 120.0);
    assert(parse_duration_to_seconds("1h") == 3600.0);
    assert(parse_duration_to_seconds("300+2") == 300.0);
    assert(parse_duration_to_seconds("5m+3") == 300.0);
    std::cout << "duration tests passed\n";
    return 0;
}
