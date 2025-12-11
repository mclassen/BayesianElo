#include "chunk_splitter.h"

#include <filesystem>
#include <fstream>
#include <string_view>

namespace bayeselo {

namespace {
constexpr std::size_t minimal_chunk = 4096;
}

std::vector<Chunk> ChunkSplitter::split(const std::string& path, std::size_t chunk_target_size) const {
    std::filesystem::path p{path};
    auto file_size = std::filesystem::file_size(p);
    std::size_t target = std::max(chunk_target_size, minimal_chunk);
    std::vector<Chunk> chunks;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return chunks;
    }

    std::string buffer(file_size, '\0');
    in.read(buffer.data(), static_cast<std::streamsize>(file_size));
    std::size_t start = 0;
    while (start < buffer.size()) {
        std::size_t end = std::min(buffer.size(), start + target);
        if (end < buffer.size()) {
            auto next = buffer.find("[Event", end);
            if (next != std::string::npos) {
                end = next;
            } else {
                end = buffer.size();
            }
        }
        chunks.push_back(Chunk{path, start, end - start});
        start = end;
    }
    return chunks;
}

}  // namespace bayeselo
