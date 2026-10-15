# Generator notes

## Provenance

The implementation is extracted from `SourceCode/handheld`, whose
`SharedConstants.cpp` identifies it as `v0.6.1 alpha`.

The vendored tree under `vendor/mcpe` contains the original headless dedicated
server compilation closure. The generator entry points are:

- `world/level/levelgen/RandomLevelSource.cpp`
- `world/level/levelgen/feature/*.h`
- `world/level/levelgen/synth/*`
- `world/level/biome/*`
- `world/level/chunk/*`
- `world/level/Level.cpp`
- `world/level/tile/*` and `world/level/material/*`

The 2013 classes are monolithic: `Level`, `Tile::initTiles`, and
`Item::initItems` reference gameplay types even when the census only invokes
generation. Those dependency objects and a small RakNet serialization support
set are compiled to preserve the original implementations, but the CLI does
not initialize networking, rendering, storage, or game assets.

`vendor/mcpe/sources.cmake` records the extracted translation units.
`tools/vendor_source.py` records how the source snapshot was selected from the
supplied archive. Do not rerun it over a modified vendor tree without also
reapplying the adaptations listed below.

## Intentional adaptations

The generation algorithms and feature order through the diamond vein are
unchanged. The extraction makes these portability changes:

1. Seed storage and RNG state use explicit 32-bit integer types. The original
   Android/Win32 targets used 32-bit `long`; Linux x86_64 uses a 64-bit `long`.
2. Biome, terrain, and population seed mixing uses explicit unsigned 32-bit
   wraparound before conversion back to a signed seed. This replaces historical
   signed-overflow behavior with its intended deterministic result.
3. Legacy logging is disabled so stdout remains valid JSON.
4. An in-memory Creative `Level` is used. Creative disables post-generation
   mob spawning; terrain and block population are the same.
5. The wrapper exposes read-only chunk bytes, hashes, placement counts, and
   diamond coordinates.
6. Chunk population continues through the lava spring. Lapis, trees, flowers,
   reeds, and cacti stay in front of the springs because they consume the
   population stream and edit the blocks a spring is allowed to replace.
   Snow and mobs are still omitted. Cave, canyon, dungeon, and lake features
   are not part of this generator.
7. Successful ore and spring writes increment a per-level counter array
   indexed by tile id. Diamond lookups use id `56`. Water and lava spring
   lookups use the ids written by `SpringFeature`, `8` and `10`. Placements
   outside the finite world are not counted. The counters do not change the
   random stream or the blocks that are written.

## Generation flow

For each seed:

1. Initialize materials, blocks, items, biomes, and tile entities once.
2. Construct `Level` with `MemoryLevelStorage`.
3. Request chunks `(0,0)` through `(15,15)` in the original x-major order
   (`x` outer, `z` inner).
4. `ChunkCache` creates base terrain with `RandomLevelSource::getChunk`.
5. Once the southeast-neighbor gate is satisfied, `postProcess` adds clay,
   ore veins through diamond, then lapis, vegetation, and water and lava
   springs.
6. Read the placement counters filled while ores and springs were written.
   A diamond total is the id `56` counter. Tests still compare it with a
   scan of the finished chunks.

The `ChunkCache` gate requires `(x+1,z)`, `(x,z+1)`, and `(x+1,z+1)`.
Population features use world coordinates and can cross a chunk boundary.
Generating a target chunk alone is therefore not equivalent to reading that
chunk from a completed world.

## Historical details that affect a census

- The finite cache is 16×16 chunks, not 256×256 chunks.
- The world block bounds are `x,z = 0..255`, `y = 0..127`.
- Out-of-world reads return invisible bedrock id `95`; writes are discarded.
- Diamond ore is the field `Tile::emeraldOre`, registered as block id `56`.
- Diamond placement occurs after clay, dirt, gravel, coal, iron, gold, and
  redstone, so all preceding RNG consumption and block replacement matters.
- Water and lava springs run after the diamond vein in the same chunk, but a
  finished chunk places them with a +8 block offset, so they land in the
  southeast neighbor before that neighbor's clay. Extra or missing water
  changes whether clay draws its vein and shifts every later ore, including
  diamond. Lapis, trees, flowers, reeds, and cacti are restored with the
  springs so that stream is the original one.
- Snow and mobs still run after the springs and are not part of this
  generator. Earlier ore veins still cross chunk borders, so the original
  traversal is retained.
- Cave, canyon, and dungeon generation is not part of this generator. The
  cave pass was already commented out in the active chunk path.
- The generator-version constructor argument is accepted but unused by
  `RandomLevelSource`.

## Validation

CTest covers MT19937 reference output, dimensions/indexing, finite boundary
behavior, raw export size, coordinate scans, per-chunk/world count agreement,
and fixed world goldens.

Current regression values:

- Seed `0`: 1,055 diamond blocks, 2 water springs, 0 lava springs; chunk
  `(0,0)` FNV-1a hash `16926736103667789954`.
- Seed `-1`: 1,033 diamond blocks, 0 water springs, 0 lava springs; chunk
  `(0,0)` FNV-1a hash `5653855523172728039`.

Seed `0` was also compared before and after extraction against a build directly
from `SourceCode/handheld`; both produced 1,055 diamond blocks. The chunk
hashes above include lapis, vegetation, and springs. They still omit snow
and mobs, so they are not hashes of a complete decoration pass.

These are regression checks for this supplied source tree. The archive includes
no official Mojang seed fixtures, so comparison with a real 0.6.1 save remains
the best independent parity test.

## Threading and repeated seeds

Legacy generation uses global block registries and mutable static generation
flags. `WorldGenerator` serializes construction, but concurrent reads from a
single instance are not advertised as thread-safe. For parallel census work,
use process-level parallelism unless the legacy state is isolated further.
