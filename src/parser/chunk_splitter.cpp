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
        // Move to next [Event boundary
        while (std::getline(in, line)) {
            pos = in.tellg();
            if (!line.empty() && line.rfind("[Event ", 0) == 0) {
                auto event_pos = static_cast<std::size_t>(line_start);
                if (event_pos > current_start) {
                    tentative_end = event_pos;
                }
                break;
            }
            if (pos == std::streampos(-1)) {
                break;
            }
            tentative_end = static_cast<std::size_t>(pos);
            line_start = pos;
        }
        if (pos == std::streampos(-1)) {
            tentative_end = total;
        }
        tentative_end = std::min(tentative_end, total);
        ranges.push_back(ChunkRange{file, current_start, tentative_end});
        current_start = tentative_end;
    }

    return ranges;
}

} // namespace bayeselo
