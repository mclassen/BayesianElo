#include "chunk_splitter.h"

#include <fstream>
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
        // Move to next [Event boundary
        while (std::getline(in, line)) {
            if (!line.empty() && line.front() == '[' && line.find("[Event") != std::string::npos) {
                break;
            }
            tentative_end = static_cast<std::size_t>(in.tellg());
        }
        ranges.push_back(ChunkRange{file, current_start, tentative_end});
        current_start = tentative_end;
    }

    return ranges;
}

} // namespace bayeselo

