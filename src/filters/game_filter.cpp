#include "game_filter.h"

#include <algorithm>

namespace bayeselo {

namespace {
bool contains_case_insensitive(const std::string& text, const std::string& query) {
    auto it = std::search(text.begin(), text.end(), query.begin(), query.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
    return it != text.end();
}
}

GameFilter::GameFilter(FilterOptions options) : options_(std::move(options)) {}

bool GameFilter::match_name(const std::string& player, const std::optional<std::string>& query) {
    if (!query) return true;
    return contains_case_insensitive(player, *query);
}

bool GameFilter::operator()(const Game& game) const {
    if (options_.require_complete) {
        if (game.tags.white.empty() || game.tags.black.empty() || game.tags.result.empty()) return false;
    }
    if (options_.skip_empty && game.tags.result == "*") return false;

    if (options_.min_plies && game.ply_count < *options_.min_plies) return false;
    if (options_.max_plies && game.ply_count > *options_.max_plies) return false;

    if (options_.min_time) {
        if (!game.duration_seconds || *game.duration_seconds < *options_.min_time) return false;
    }
    if (options_.max_time) {
        if (!game.duration_seconds || *game.duration_seconds > *options_.max_time) return false;
    }

    if (!match_name(game.tags.white, options_.white_name)) return false;
    if (!match_name(game.tags.black, options_.black_name)) return false;
    if (options_.either_name) {
        if (!match_name(game.tags.white, options_.either_name) && !match_name(game.tags.black, options_.either_name)) return false;
    }
    if (options_.exclude_name) {
        if (match_name(game.tags.white, options_.exclude_name) || match_name(game.tags.black, options_.exclude_name)) return false;
    }

    if (options_.result) {
        if (*options_.result != "any") {
            if (*options_.result == "draw" && game.tags.result != "1/2-1/2") return false;
            else if (*options_.result == "1-0" && game.tags.result != "1-0") return false;
            else if (*options_.result == "0-1" && game.tags.result != "0-1") return false;
        }
    }

    if (options_.termination && game.tags.termination) {
        if (!contains_case_insensitive(*game.tags.termination, *options_.termination)) return false;
    }

    return true;
}

}  // namespace bayeselo
