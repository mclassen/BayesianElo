#pragma once

#include "bayeselo/fastchess_stats.h"
#include "bayeselo/rating_result.h"

#include <string>

namespace bayeselo {

void print_ratings(const RatingResult& result, std::size_t planned_games = 0);
void print_los_matrix(const RatingResult& result);
void print_ratings_markdown(const RatingResult& result, std::size_t planned_games = 0);
void print_los_matrix_markdown(const RatingResult& result);
void print_fastchess_head_to_head(const FastchessHeadToHeadStats& stats, std::size_t planned_games = 0);
void print_fastchess_head_to_head_markdown(const FastchessHeadToHeadStats& stats, std::size_t planned_games = 0);

} // namespace bayeselo
