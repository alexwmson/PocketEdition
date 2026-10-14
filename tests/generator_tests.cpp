#include "generator/WorldGenerator.h"
#include "util/Random.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        Random random(5489);
        expect(random.nextInt() == 1749605806,
               "MT19937 first value differs from the 0.6.1 implementation");
        expect(random.nextInt() == 290934651,
               "MT19937 second value differs from the 0.6.1 implementation");

        expect(mcpe::WorldGenerator::kChunkBlockCount == 32768,
               "chunk block count must be 32768");
        expect(mcpe::WorldGenerator::blockIndex(15, 127, 15) == 32767,
               "chunk layout maximum index is wrong");

        const mcpe::WorldGenerator world(0);
        expect(world.isGeneratedChunk(15, 15), "chunk 15,15 must be generated");
        expect(!world.isGeneratedChunk(16, 16), "padding chunks must be empty");
        expect(world.countDiamonds(16, 16) == 0,
               "padding chunks cannot contain diamonds");
        expect(std::count(
                   world.chunkData(25, 25),
                   world.chunkData(25, 25) + mcpe::WorldGenerator::kChunkBlockCount,
                   mcpe::WorldGenerator::kOutsideWorldId) ==
                   static_cast<std::ptrdiff_t>(mcpe::WorldGenerator::kChunkBlockCount),
               "padding chunks must use invisible bedrock");
        expect(world.countDiamonds() == 1055,
               "seed 0 diamond golden count changed");
        expect(world.chunkHash(0, 0) == UINT64_C(16926736103667789954),
               "seed 0 chunk hash changed");
        expect(world.chunkHash(2, 0) == UINT64_C(12054506634790722117),
               "seed 0 populated chunk hash changed");
        expect(world.chunkHash(15, 15) == UINT64_C(11408663654075882097),
               "seed 0 far-edge chunk hash changed");

        std::size_t sum = 0;
        for (int z = 0; z < mcpe::WorldGenerator::kWorldChunks; ++z) {
            for (int x = 0; x < mcpe::WorldGenerator::kWorldChunks; ++x) {
                sum += world.countDiamonds(x, z);
            }
        }
        expect(sum == world.countDiamonds(),
               "world count must equal per-chunk sum");

        const std::vector<mcpe::DiamondBlock> positions = world.diamonds();
        expect(positions.size() == world.countDiamonds(),
               "diamond coordinate count differs from block scan");
        for (std::size_t i = 0; i < positions.size(); ++i) {
            expect(positions[i].x >= 0 && positions[i].x < 256,
                   "diamond x coordinate is outside the finite world");
            expect(positions[i].y >= 0 && positions[i].y < 128,
                   "diamond y coordinate is outside build height");
            expect(positions[i].z >= 0 && positions[i].z < 256,
                   "diamond z coordinate is outside the finite world");
        }

        const mcpe::WorldGenerator negativeWorld(-1);
        expect(negativeWorld.countDiamonds() == 1033,
               "seed -1 diamond golden count changed");
        expect(negativeWorld.chunkHash(0, 0) == UINT64_C(5653855523172728039),
               "seed -1 chunk hash changed");
    } catch (const std::exception& error) {
        std::cerr << "test failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
