#pragma once

#include "bayeselo/rating_result.h"

#include <filesystem>
#include <optional>

namespace bayeselo {

void write_csv(const RatingResult& result, const std::filesystem::path& path);
void write_json(const RatingResult& result, const std::filesystem::path& path);

}

