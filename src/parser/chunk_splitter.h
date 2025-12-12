#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace bayeselo {

struct ChunkRange {
    std::filesystem::path file;
    std::size_t start_offset{0};
    std::size_t end_offset{0};
};

std::vector<ChunkRange> split_pgn_file(const std::filesystem::path& file, std::size_t chunk_bytes);

} // namespace bayeselo
