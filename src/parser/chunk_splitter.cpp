#include "chunk_splitter.h"

#include <fstream>
#include <string>
#include <vector>

namespace bayeselo {

std::vector<ChunkRange> split_pgn_file(const std::filesystem::path& file, std::size_t chunk_bytes) {
    std::vector<ChunkRange> ranges;
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        return ranges;
    }

    in.seekg(0, std::ios::end);
    const auto total = static_cast<std::size_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    std::size_t current_start = 0;
    while (current_start < total) {
        std::size_t tentative_end = std::min(current_start + chunk_bytes, total);
        in.seekg(static_cast<std::streamoff>(tentative_end), std::ios::beg);
        std::string line;
        std::streampos line_start = in.tellg();
        std::streampos pos = line_start;
        std::size_t last_pos = tentative_end;
        bool found_event = false;
        std::streampos prev_pos = pos;
        // Move to next [Event] boundary
        while (std::getline(in, line)) {
            pos = in.tellg();
            if (pos == prev_pos || in.eof() || in.fail()) {
                break; // no forward progress; bail out to avoid infinite loop
            }
            prev_pos = pos;
            if (!line.empty() && line.rfind("[Event", 0) == 0) {
                char after = line.size() > 6 ? line[6] : '\0';
                if (after != '\0' && after != '"' && !std::isspace(static_cast<unsigned char>(after))) {
                    line_start = pos;
                    continue;
                }
                auto event_pos = static_cast<std::size_t>(line_start);
                if (event_pos > current_start) {
                    tentative_end = event_pos;
                }
                found_event = true;
                break;
            }
            if (in.eof() || in.fail()) {
                break;
            }
            last_pos = static_cast<std::size_t>(pos);
            line_start = pos;
        }
        if (!found_event && (in.eof() || in.fail())) {
            tentative_end = total;
        } else if (!found_event) {
            tentative_end = last_pos;
        }
        tentative_end = std::min(tentative_end, total);
        ranges.push_back(ChunkRange{file, current_start, tentative_end});
        current_start = tentative_end;
    }

    return ranges;
}

} // namespace bayeselo
