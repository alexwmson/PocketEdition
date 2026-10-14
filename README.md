# MCPE 0.6.1 Chunk Census

This project builds a headless, asset-free generator from the supplied
Minecraft: Pocket Edition 0.6.1 source. It generates the complete finite
overworld in memory, exports final chunk block IDs, and counts diamond ore
(block id `56`, historically named `emeraldOre` in the code).

## Build on Ubuntu

Requirements: CMake 3.16+ and a GNU C++ compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

No assets, game installation, network service, or external library download is
required.

## Commands

Count every diamond ore block in a seed:

```bash
./build/mcpe-census count 0
./build/mcpe-census count 0 --per-chunk
```

Export one chunk as 32,768 raw block-id bytes:

```bash
./build/mcpe-census chunk 0 2 0 --output seed0-2-0.bin
```

List diamond coordinates for a world or one chunk:

```bash
./build/mcpe-census diamonds 0
./build/mcpe-census diamonds 0 2 0
```

All stdout results are JSON. Errors and help text use stderr where appropriate.
Seeds must be signed 32-bit integers (`-2147483648` through `2147483647`).

## Dimensions and chunk format

MCPE 0.6.1 uses a finite world:

- 256×256 blocks horizontally
- 128 blocks high (`y = 0..127`)
- 16×16 generated chunks (`chunk x,z = 0..15`)
- 16×16×128 = 32,768 block IDs per chunk

The CLI accepts chunk coordinates `0..25` as requested. Coordinates `16..25`
are not extra terrain: the original game treats them as its shared boundary
chunk. They export as 32,768 bytes of invisible bedrock (id `95`) and report
`"generated": false`.

Raw chunk bytes use the original array index:

```text
index = (localX << 11) | (localZ << 7) | y
```

`localX` and `localZ` are `0..15`; `y` is `0..127`.

## Why one chunk generates the world

The terrain base is generated per chunk, but population is not isolated.
Ores, trees, and springs can write into adjacent chunks, and the original
`ChunkCache` starts population only after southeast neighbors exist. To return
the same final blocks as a normal finite world, every command generates all
16×16 real chunks in the original x-major traversal before reading results.

On a typical desktop this takes a few seconds per seed. Reuse
`mcpe::WorldGenerator` from `src/generator/WorldGenerator.h` when making
multiple queries against one seed.

## Known behavior

- Caves are disabled because their call is commented out in this source
  revision.
- Diamond generation makes one size-7 ore-feature attempt per populated
  chunk, anchored at `y = 0..15`; resulting vein blocks can extend slightly
  higher and across chunk boundaries.
- Writes outside the finite 256×256 world are discarded by the original empty
  boundary chunk.
- Generation uses the source's MT19937 RNG, not Java Edition's LCG.
- Mob spawning is disabled through Creative world settings. It occurs after
  block population and does not affect generated blocks.

See `docs/GENERATOR_NOTES.md` for provenance and implementation details.

## Source notice

The supplied archive does not contain an open-source license grant for the
Minecraft source. The extracted files retain their original notices where
present. Review your rights before redistributing this project or binaries.
