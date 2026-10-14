#include "generator/WorldGenerator.h"

#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <string>

#include "SharedConstants.h"
#include "util/Mth.h"
#include "world/entity/MobCategory.h"
#include "world/item/Item.h"
#include "world/level/Level.h"
#include "world/level/LevelSettings.h"
#include "world/level/biome/Biome.h"
#include "world/level/chunk/LevelChunk.h"
#include "world/level/material/Material.h"
#include "world/level/storage/MemoryLevelStorage.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/entity/TileEntity.h"

namespace mcpe {
namespace {

std::once_flag runtimeInitFlag;
std::mutex generationMutex;

void initializeLegacyRuntime() {
    Mth::initMth();
    Material::initMaterials();
    MobCategory::initMobCategories();
    Tile::initTiles();
    Item::initItems();
    Biome::initBiomes();
    TileEntity::initTileEntities();
}

} // namespace

const std::uint8_t WorldGenerator::kDiamondOreId;
const std::uint8_t WorldGenerator::kOutsideWorldId;

struct WorldGenerator::Impl {
    explicit Impl(std::int32_t worldSeed)
        : seed(worldSeed),
          storage(),
          level(new Level(&storage,
                          "census",
                          LevelSettings(worldSeed, GameType::Creative),
                          SharedConstants::GeneratorVersion)) {}

    std::int32_t seed;
    MemoryLevelStorage storage;
    std::unique_ptr<Level> level;
};

WorldGenerator::WorldGenerator(std::int32_t seed)
    : impl_(), outsideChunk_() {
    std::call_once(runtimeInitFlag, initializeLegacyRuntime);
    std::lock_guard<std::mutex> lock(generationMutex);

    outsideChunk_.fill(kOutsideWorldId);
    impl_.reset(new Impl(seed));

    // This matches Minecraft::prepareLevel's x-major finite-world traversal.
    // Loading the complete grid is necessary because population can write
    // across a chunk edge and ChunkCache uses a southeast-neighbor gate.
    for (int chunkX = 0; chunkX < kWorldChunks; ++chunkX) {
        for (int chunkZ = 0; chunkZ < kWorldChunks; ++chunkZ) {
            impl_->level->getChunk(chunkX, chunkZ);
        }
    }

    for (int chunkX = 0; chunkX < kWorldChunks; ++chunkX) {
        for (int chunkZ = 0; chunkZ < kWorldChunks; ++chunkZ) {
            if (!impl_->level->getChunk(chunkX, chunkZ)->terrainPopulated) {
                throw std::runtime_error("legacy generator left a chunk unpopulated");
            }
        }
    }
}

WorldGenerator::~WorldGenerator() = default;

std::int32_t WorldGenerator::seed() const {
    return impl_->seed;
}

void WorldGenerator::validateQueryableChunk(int chunkX, int chunkZ) {
    if (chunkX < 0 || chunkZ < 0 ||
        chunkX >= kQueryableChunks || chunkZ >= kQueryableChunks) {
        throw std::out_of_range("chunk coordinates must be in the range 0..25");
    }
}

bool WorldGenerator::isGeneratedChunk(int chunkX, int chunkZ) const {
    validateQueryableChunk(chunkX, chunkZ);
    return chunkX < kWorldChunks && chunkZ < kWorldChunks;
}

const std::uint8_t* WorldGenerator::chunkData(int chunkX, int chunkZ) const {
    validateQueryableChunk(chunkX, chunkZ);
    if (!isGeneratedChunk(chunkX, chunkZ)) {
        return outsideChunk_.data();
    }
    return impl_->level->getChunk(chunkX, chunkZ)->getBlockData();
}

std::size_t WorldGenerator::blockIndex(int localX, int y, int localZ) {
    if (localX < 0 || localX >= kChunkWidth ||
        localZ < 0 || localZ >= kChunkWidth ||
        y < 0 || y >= kWorldHeight) {
        throw std::out_of_range("block coordinates are outside a chunk");
    }
    return static_cast<std::size_t>((localX << 11) | (localZ << 7) | y);
}

std::size_t WorldGenerator::countDiamonds(int chunkX, int chunkZ) const {
    const std::uint8_t* blocks = chunkData(chunkX, chunkZ);
    return static_cast<std::size_t>(
        std::count(blocks, blocks + kChunkBlockCount, kDiamondOreId));
}

std::size_t WorldGenerator::countDiamonds() const {
    std::size_t total = 0;
    for (int chunkZ = 0; chunkZ < kWorldChunks; ++chunkZ) {
        for (int chunkX = 0; chunkX < kWorldChunks; ++chunkX) {
            total += countDiamonds(chunkX, chunkZ);
        }
    }
    return total;
}

std::vector<DiamondBlock> WorldGenerator::diamonds(int chunkX, int chunkZ) const {
    const std::uint8_t* blocks = chunkData(chunkX, chunkZ);
    std::vector<DiamondBlock> result;
    if (!isGeneratedChunk(chunkX, chunkZ)) {
        return result;
    }

    for (int localX = 0; localX < kChunkWidth; ++localX) {
        for (int localZ = 0; localZ < kChunkWidth; ++localZ) {
            for (int y = 0; y < kWorldHeight; ++y) {
                if (blocks[blockIndex(localX, y, localZ)] == kDiamondOreId) {
                    DiamondBlock block = {
                        chunkX * kChunkWidth + localX,
                        y,
                        chunkZ * kChunkWidth + localZ,
                    };
                    result.push_back(block);
                }
            }
        }
    }
    return result;
}

std::vector<DiamondBlock> WorldGenerator::diamonds() const {
    std::vector<DiamondBlock> result;
    for (int chunkZ = 0; chunkZ < kWorldChunks; ++chunkZ) {
        for (int chunkX = 0; chunkX < kWorldChunks; ++chunkX) {
            std::vector<DiamondBlock> chunk = diamonds(chunkX, chunkZ);
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
    }
    return result;
}

std::uint64_t WorldGenerator::chunkHash(int chunkX, int chunkZ) const {
    const std::uint8_t* blocks = chunkData(chunkX, chunkZ);
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (std::size_t i = 0; i < kChunkBlockCount; ++i) {
        hash ^= blocks[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

} // namespace mcpe
