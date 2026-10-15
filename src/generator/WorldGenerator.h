#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace mcpe {

struct DiamondBlock {
    int x;
    int y;
    int z;
};

class WorldGenerator {
public:
    static const int kWorldChunks = 16;
    static const int kQueryableChunks = 26;
    static const int kChunkWidth = 16;
    static const int kWorldHeight = 128;
    static const std::size_t kChunkBlockCount =
        kChunkWidth * kChunkWidth * kWorldHeight;
    static const int kTileIdCount = 256;
    static const std::uint8_t kDiamondOreId = 56;
    static const std::uint8_t kWaterSpringId = 8;
    static const std::uint8_t kLavaSpringId = 10;
    static const std::uint8_t kOutsideWorldId = 95;

    explicit WorldGenerator(std::int32_t seed);
    ~WorldGenerator();

    WorldGenerator(const WorldGenerator&) = delete;
    WorldGenerator& operator=(const WorldGenerator&) = delete;

    std::int32_t seed() const;
    bool isGeneratedChunk(int chunkX, int chunkZ) const;
    const std::uint8_t* chunkData(int chunkX, int chunkZ) const;
    std::size_t count(int tileId) const;
    std::size_t count(int tileId, int chunkX, int chunkZ) const;
    std::size_t countDiamonds() const;
    std::size_t countDiamonds(int chunkX, int chunkZ) const;
    std::vector<DiamondBlock> diamonds() const;
    std::vector<DiamondBlock> diamonds(int chunkX, int chunkZ) const;
    std::uint64_t chunkHash(int chunkX, int chunkZ) const;

    static std::size_t blockIndex(int localX, int y, int localZ);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::array<std::uint8_t, kChunkBlockCount> outsideChunk_;

    static void validateQueryableChunk(int chunkX, int chunkZ);
};

} // namespace mcpe
