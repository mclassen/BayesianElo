#include <iostream>

bool test_parser();
bool test_filters();
bool test_rating();

int main() {
    std::cout << "Running parser test...\n";
    if (!test_parser()) return 1;
    std::cout << "Running filter test...\n";
    if (!test_filters()) return 1;
    std::cout << "Running rating test...\n";
    if (!test_rating()) return 1;
    std::cout << "All tests passed\n";
    return 0;
}
