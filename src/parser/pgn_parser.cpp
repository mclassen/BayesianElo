#include "pgn_parser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <mutex>
#include <ranges>
#include <sstream>

#include "../util/thread_pool.h"

namespace bayeselo {

namespace {
std::string trim(std::string_view v) {
    while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.remove_prefix(1);
    while (!v.empty() && (v.back() == ' ' || v.back() == '\t' || v.back() == '\r')) v.remove_suffix(1);
    return std::string(v);
}

std::optional<std::uint32_t> parse_duration(const std::string& time_control) {
    if (time_control.empty()) return std::nullopt;
    std::uint32_t value = 0;
    std::string number;
    std::string suffix;
    for (char c : time_control) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            number.push_back(c);
        } else if (!number.empty()) {
            suffix.push_back(c);
        }
    }
    if (number.empty()) return std::nullopt;
    value = static_cast<std::uint32_t>(std::stoul(number));
    if (suffix == "m") value *= 60;
    else if (suffix == "h") value *= 3600;
    return value;
}
}

PgnParser::PgnParser(ParseOptions opts, GameFilter filter) : options_(opts), filter_(std::move(filter)) {}

Game PgnParser::parse_game(const std::vector<std::string>& lines) const {
    GameTags tags;
    std::vector<std::string> moves;
    std::uint32_t ply_count = 0;
    for (const auto& line : lines) {
        if (line.starts_with('[')) {
            parse_tags(line, tags);
        } else {
            parse_moves(line, moves, ply_count);
        }
    }
    Game g{tags, moves, ply_count, std::nullopt};
    if (tags.time_control) {
        g.duration_seconds = parse_duration(*tags.time_control);
    }
    return g;
}

void PgnParser::parse_tags(const std::string& line, GameTags& tags) const {
    auto content = line;
    if (content.size() < 5) return;
    auto start = content.find('[');
    auto end = content.rfind(']');
    if (start == std::string::npos || end == std::string::npos || end <= start) return;
    auto inner = content.substr(start + 1, end - start - 1);
    auto first_space = inner.find(' ');
    if (first_space == std::string::npos) return;
    auto tag = inner.substr(0, first_space);
    auto value = trim(inner.substr(first_space + 1));
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    if (tag == "White") tags.white = value;
    else if (tag == "Black") tags.black = value;
    else if (tag == "Result") tags.result = value;
    else if (tag == "Termination") tags.termination = value;
    else if (tag == "UTCDate") tags.utc_date = value;
    else if (tag == "UTCTime") tags.utc_time = value;
    else if (tag == "TimeControl") tags.time_control = value;
}

void PgnParser::parse_moves(const std::string& line, std::vector<std::string>& moves, std::uint32_t& ply_count) const {
    std::string token;
    bool in_comment = false;
    int paren_depth = 0;
    for (char c : line) {
        if (c == '{') { in_comment = true; continue; }
        if (c == '}') { in_comment = false; continue; }
        if (c == '(') { ++paren_depth; continue; }
        if (c == ')') { if (paren_depth > 0) --paren_depth; continue; }
        if (in_comment || paren_depth > 0) continue;
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                moves.push_back(token);
                ++ply_count;
                token.clear();
            }
        } else if (std::isdigit(static_cast<unsigned char>(c)) && token.empty()) {
            // move number prefix, skip
        } else if (c == '.') {
            continue;
        } else {
            token.push_back(c);
        }
    }
    if (!token.empty()) {
        moves.push_back(token);
        ++ply_count;
    }
}

std::vector<Game> PgnParser::parse_files(const std::vector<std::string>& files) const {
    ChunkSplitter splitter;
    std::mutex guard;
    std::vector<Game> games;

    auto process_chunk = [&](const Chunk& chunk) {
        std::ifstream in(chunk.path, std::ios::binary);
        if (!in) return;
        in.seekg(static_cast<std::streamoff>(chunk.offset));
        std::string buffer(chunk.length, '\0');
        in.read(buffer.data(), static_cast<std::streamsize>(chunk.length));
        std::istringstream stream(buffer);
        std::string line;
        std::vector<std::string> current;
        while (std::getline(stream, line)) {
            if (line.rfind("[Event", 0) == 0 && !current.empty()) {
                auto game = parse_game(current);
                if (filter_(game)) {
                    std::scoped_lock lock(guard);
                    games.push_back(std::move(game));
                }
                current.clear();
            }
            current.push_back(line);
        }
        if (!current.empty()) {
            auto game = parse_game(current);
            if (filter_(game)) {
                std::scoped_lock lock(guard);
                games.push_back(std::move(game));
            }
        }
    };

    if (options_.threads <= 1) {
        for (const auto& file : files) {
            auto chunks = splitter.split(file, options_.chunk_size);
            for (const auto& chunk : chunks) {
                process_chunk(chunk);
            }
        }
    } else {
        ThreadPool pool(options_.threads);
        for (const auto& file : files) {
            auto chunks = splitter.split(file, options_.chunk_size);
            for (const auto& chunk : chunks) {
                pool.enqueue([&, chunk]() { process_chunk(chunk); });
            }
        }
        pool.wait();
    }
    return games;
}

}  // namespace bayeselo
