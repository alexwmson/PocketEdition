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

Count placements in a seed. The default kind is diamonds. `watersprings` and
`lavasprings` count successful spring placements, and `springs` adds those
together:

```bash
./build/mcpe-census count 0
./build/mcpe-census count 0 --per-chunk
./build/mcpe-census count --range 0-9
./build/mcpe-census count --range 0-9 --per-chunk
./build/mcpe-census count 0 --type watersprings
./build/mcpe-census count --range 0-9 --type springs
```

`--range` is inclusive and counts every seed in one process. Each seed is one
JSON object on its own line, in the same shape as a single-seed count.
Negative bounds keep their sign: `--range -10--1` or `--range -10..-1`.

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

On a typical desktop this takes a few seconds per seed. `count --range` still
spends that time generating each world, but it initializes the runtime once
and avoids starting a new process per seed. Reuse `mcpe::WorldGenerator` from
`src/generator/WorldGenerator.h` when making multiple queries against one seed.

## Known behavior

- Population runs through water and lava springs. Lapis, trees, flowers,
  reeds, and cacti are placed first because they consume the same random
  stream and can change the blocks a spring replaces. Snow and mobs are not
  placed. Caves, ravines, and dungeons are not generated. Clay and the earlier
  ore veins (dirt, gravel, coal, iron, gold, and redstone) are unchanged:
  they consume the same random numbers and can replace stone the diamond vein
  would otherwise claim.
- Springs from an earlier chunk can add or remove water before a later
  chunk's clay. That is why they are included even though they follow the
  diamond vein inside one chunk. Seeds `0` and `-1` still have 1,055 and
  1,033 diamond blocks.
- Diamond generation makes one size-7 ore-feature attempt per populated
  chunk, anchored at `y = 0..15`; resulting vein blocks can extend slightly
  higher and across chunk boundaries. The diamond total is the number of
  those blocks recorded as they replace stone, looked up by tile id `56`.
- Water and lava spring totals are also recorded at placement, under tile
  ids `8` and `10`. Each populated chunk attempts 50 water springs and 20
  lava springs, and only a stone pocket with three stone sides and one open
  side is counted. Flowing liquid left behind by the spring tick is not
  counted again. Ocean water is not a spring. Seed `0` has 2 water springs
  and no lava springs. Seed `-1` has neither. Caves are not generated, so
  almost no stone pocket has exactly one open side.
- Writes outside the finite 256×256 world are discarded by the original empty
  boundary chunk.
- Generation uses the source's MT19937 RNG, not Java Edition's LCG.
- Chunk hashes include lapis, vegetation, and springs. They still differ from
  a full population where snow or a mob would have changed a block.

See `docs/GENERATOR_NOTES.md` for provenance and implementation details.

## Source notice

The supplied archive does not contain an open-source license grant for the
Minecraft source. The extracted files retain their original notices where
present. Review your rights before redistributing this project or binaries.
