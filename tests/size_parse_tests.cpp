#include "bayeselo/size_parse.h"

#include <iostream>
#include <limits>
#include <string>

int main() {
    using bayeselo::parse_size;
    auto fail = [&](const std::string& msg) {
        std::cerr << msg << "\n";
        return 1;
    };

    auto v1 = parse_size("1k");
    if (!v1 || *v1 != 1024) return fail("expected 1k == 1024");
    auto v2 = parse_size("2M");
    if (!v2 || *v2 != 2ull * 1024ull * 1024ull) return fail("expected 2M == 2 MiB");

    if (parse_size("-1")) return fail("expected -1 rejected");
    if (parse_size("-5g")) return fail("expected -5g rejected");
    if (parse_size("foo")) return fail("expected foo rejected");
    if (parse_size("10t")) return fail("expected invalid suffix rejected");

    const auto max_sz = std::numeric_limits<std::size_t>::max();
    auto overflow = parse_size(std::to_string(max_sz) + "k");
    if (overflow) return fail("expected overflow rejected");

    std::cout << "size parse tests passed\n";
    return 0;
}
