#include "pgn_parser.h"

#include "bayeselo/duration.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace bayeselo {

namespace {
std::optional<std::string> parse_tag_line(std::string_view line) {
    if (line.size() < 2 || line.front() != '[' || line.back() != ']') {
        return std::nullopt;
    }
    return std::string(line.substr(1, line.size() - 2));
}

std::pair<std::string, std::string> split_tag(std::string_view tag_line) {
    auto space = tag_line.find(' ');
    if (space == std::string_view::npos) {
        return {std::string(tag_line), {}};
    }
    auto key = tag_line.substr(0, space);
    auto value_view = tag_line.substr(space + 1);
    if (!value_view.empty() && value_view.front() == '"' && value_view.back() == '"') {
        value_view = value_view.substr(1, value_view.size() - 2);
    }
    return {std::string(key), std::string(value_view)};
}

std::vector<std::string> tokenize_moves(const std::string& text) {
    constexpr std::size_t kMinMoveReserve = 4;
    constexpr std::size_t kMinTokenReserve = 4;
    std::vector<std::string> moves;
    // Reserve roughly half the characters for move tokens, but keep a small floor to avoid zero-capacity reserve.
    moves.reserve(std::max<std::size_t>(kMinMoveReserve, text.size() / 2));
    std::string token;
    // Tokens tend to be shorter than moves, so reserve ~1/4 of the text with a minimum to avoid immediate reallocs.
    token.reserve(std::max<std::size_t>(kMinTokenReserve, text.size() / 4));
    bool in_comment = false;
    int variation_depth = 0;

    auto flush_token = [&]() {
        if (!token.empty()) {
            moves.push_back(token);
            token.clear();
        }
    };

    for (unsigned char ch : text) {
        if (ch == '{') { in_comment = true; flush_token(); continue; }
        if (ch == '}') { in_comment = false; continue; }
        if (ch == '(') { ++variation_depth; flush_token(); continue; }
        if (ch == ')') { if (variation_depth > 0) --variation_depth; continue; }
        if (in_comment || variation_depth > 0) continue;
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            flush_token();
            continue;
        }
        token.push_back(static_cast<char>(ch));
    }
    flush_token();
    return moves;
}

GameResult::Outcome outcome_from_result(const std::string& r) {
    if (r == "1-0") return GameResult::Outcome::WhiteWin;
    if (r == "0-1") return GameResult::Outcome::BlackWin;
    if (r == "1/2-1/2") return GameResult::Outcome::Draw;
    return GameResult::Outcome::Unknown;
}

} // namespace

std::optional<std::vector<Game>> parse_pgn_chunk(const std::filesystem::path& file, std::size_t start, std::size_t end) {
    std::vector<Game> games;
    if (end <= start) return games;

    std::ifstream in(file, std::ios::binary);
    if (!in) return std::nullopt;

    in.seekg(0, std::ios::end);
    const auto total = static_cast<std::size_t>(in.tellg());
    if (start >= total) return games;
    end = std::min(end, total);

    const std::size_t length = end - start;
    std::string buffer(length, '\0');
    in.seekg(static_cast<std::streamoff>(start), std::ios::beg);
    in.read(buffer.data(), static_cast<std::streamoff>(length));
    if (!in) return std::nullopt;

    Game current;
    bool in_headers = true;
    std::string move_text;

    auto flush_game = [&]() {
        current.moves = tokenize_moves(move_text);
        current.ply_count = static_cast<std::uint32_t>(current.moves.size());
        if (current.meta.time_control) {
            try {
                current.estimated_duration_seconds = parse_duration_to_seconds(*current.meta.time_control);
            } catch (const std::exception&) {
                current.estimated_duration_seconds = std::nullopt;
            }
        }
        games.push_back(std::move(current));
        current = Game{};
        move_text.clear();
        in_headers = true;
    };

    std::size_t pos = 0;
    while (pos < buffer.size()) {
        std::size_t line_end = buffer.find('\n', pos);
        if (line_end == std::string::npos) line_end = buffer.size();
        std::string_view line_view(buffer.data() + pos, line_end - pos);
        if (!line_view.empty() && line_view.back() == '\r') {
            line_view.remove_suffix(1);
        }

        if (line_view.empty()) {
            if (!in_headers) {
                flush_game();
            }
        } else if (line_view.front() == '[') {
            auto tag_line = parse_tag_line(line_view);
            if (tag_line) {
                const auto& [key, value] = split_tag(*tag_line);
                if (key == "White") current.meta.white = value;
                else if (key == "Black") current.meta.black = value;
                else if (key == "Result") current.result.outcome = outcome_from_result(value);
                else if (key == "Termination") current.result.termination = value;
                else if (key == "UTCDate") current.meta.utc_date = value;
                else if (key == "UTCTime") current.meta.utc_time = value;
                else if (key == "TimeControl") current.meta.time_control = value;
            }
            in_headers = true;
        } else {
            in_headers = false;
            move_text.append(line_view);
            move_text.push_back(' ');
        }

        if (line_end == buffer.size()) break;
        pos = line_end + 1;
    }
    if (!move_text.empty()) {
        flush_game();
    }
    return games;
}

static bool contains_case_insensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
    return it != haystack.end();
}

bool passes_filters(const Game& game, const FilterConfig& config) {
    if (config.require_complete) {
        if (game.meta.white.empty() || game.meta.black.empty()) return false;
        if (game.result.outcome == GameResult::Outcome::Unknown) return false;
    }
    if (config.skip_empty && game.result.outcome == GameResult::Outcome::Unknown) return false;

    if (config.min_plies && game.ply_count < *config.min_plies) return false;
    if (config.max_plies && game.ply_count > *config.max_plies) return false;

    if (config.min_time_seconds || config.max_time_seconds) {
        std::optional<double> duration = game.estimated_duration_seconds;
        if (!duration && game.meta.time_control) {
            try {
                duration = parse_duration_to_seconds(*game.meta.time_control);
            } catch (const std::exception&) {
                duration = std::nullopt;
            }
        }
        if (!duration) return false;
        if (config.min_time_seconds && *duration < *config.min_time_seconds) return false;
        if (config.max_time_seconds && *duration > *config.max_time_seconds) return false;
    }

    if (config.white_name && !contains_case_insensitive(game.meta.white, *config.white_name)) return false;
    if (config.black_name && !contains_case_insensitive(game.meta.black, *config.black_name)) return false;
    if (config.either_name && !(contains_case_insensitive(game.meta.white, *config.either_name) || contains_case_insensitive(game.meta.black, *config.either_name))) return false;
    if (config.exclude_name && (contains_case_insensitive(game.meta.white, *config.exclude_name) || contains_case_insensitive(game.meta.black, *config.exclude_name))) return false;

    if (config.result_filter) {
        const auto& rf = *config.result_filter;
        if (rf == "1-0" && game.result.outcome != GameResult::Outcome::WhiteWin) return false;
        if (rf == "0-1" && game.result.outcome != GameResult::Outcome::BlackWin) return false;
        if ((rf == "draw" || rf == "1/2-1/2") && game.result.outcome != GameResult::Outcome::Draw) return false;
    }

    if (config.termination) {
        if (!game.result.termination) return false;
        if (!contains_case_insensitive(*game.result.termination, *config.termination)) return false;
    }
    return true;
}

} // namespace bayeselo
