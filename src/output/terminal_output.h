#pragma once

#include <string>
#include <vector>

#include "../../include/bayeselo/game.h"

namespace bayeselo {

class TerminalOutput {
public:
    void print_summary(const RatingResult& result) const;
    void print_los(const RatingResult& result) const;
};

}  // namespace bayeselo
