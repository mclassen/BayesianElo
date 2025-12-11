#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace bayeselo {

struct Chunk {
    std::string path;
    std::size_t offset{};
    std::size_t length{};
};

class ChunkSplitter {
public:
    std::vector<Chunk> split(const std::string& path, std::size_t chunk_target_size) const;
};

}  // namespace bayeselo
