#pragma once

#include <string>

#include "../../include/bayeselo/game.h"

namespace bayeselo {

class ExportWriter {
public:
    void write_csv(const std::string& path, const RatingResult& result) const;
    void write_json(const std::string& path, const RatingResult& result) const;
};

}  // namespace bayeselo
