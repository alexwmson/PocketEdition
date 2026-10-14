#include "generator/WorldGenerator.h"

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void usage(std::ostream& output) {
    output
        << "MCPE 0.6.1 finite-world generator\n"
        << "usage:\n"
        << "  mcpe-census count <seed> [--per-chunk]\n"
        << "  mcpe-census chunk <seed> <chunk-x> <chunk-z> [--output <file>]\n"
        << "  mcpe-census diamonds <seed> [<chunk-x> <chunk-z>]\n"
        << "\n"
        << "Seeds are signed 32-bit integers. Queryable chunk coordinates are\n"
        << "0..25; only 0..15 contain generated MCPE finite-world terrain.\n";
}

long long parseInteger(const char* text, const char* name) {
    errno = 0;
    char* end = NULL;
    const long long value = std::strtoll(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        throw std::invalid_argument(std::string(name) + " must be an integer");
    }
    return value;
}

std::int32_t parseSeed(const char* text) {
    const long long value = parseInteger(text, "seed");
    if (value < INT32_MIN || value > INT32_MAX) {
        throw std::out_of_range("seed must be in signed 32-bit range");
    }
    return static_cast<std::int32_t>(value);
}

int parseChunk(const char* text, const char* name) {
    const long long value = parseInteger(text, name);
    if (value < 0 || value >= mcpe::WorldGenerator::kQueryableChunks) {
        throw std::out_of_range(std::string(name) + " must be in range 0..25");
    }
    return static_cast<int>(value);
}

std::string jsonString(const std::string& value) {
    std::string result = "\"";
    for (std::string::const_iterator it = value.begin(); it != value.end(); ++it) {
        if (*it == '\\' || *it == '"') {
            result += '\\';
        }
        result += *it;
    }
    result += '"';
    return result;
}

void printChunkCounts(const mcpe::WorldGenerator& world) {
    std::cout << ",\"chunks\":[";
    bool first = true;
    for (int z = 0; z < mcpe::WorldGenerator::kWorldChunks; ++z) {
        for (int x = 0; x < mcpe::WorldGenerator::kWorldChunks; ++x) {
            if (!first) {
                std::cout << ',';
            }
            first = false;
            std::cout << "{\"x\":" << x
                      << ",\"z\":" << z
                      << ",\"diamonds\":" << world.countDiamonds(x, z)
                      << '}';
        }
    }
    std::cout << ']';
}

int runCount(int argc, char** argv) {
    if (argc != 3 && !(argc == 4 && std::string(argv[3]) == "--per-chunk")) {
        throw std::invalid_argument("count expects <seed> [--per-chunk]");
    }
    const mcpe::WorldGenerator world(parseSeed(argv[2]));
    std::cout << "{\"version\":\"0.6.1\",\"seed\":" << world.seed()
              << ",\"diamond_block_id\":56"
              << ",\"diamonds\":" << world.countDiamonds();
    if (argc == 4) {
        printChunkCounts(world);
    }
    std::cout << "}\n";
    return 0;
}

int runChunk(int argc, char** argv) {
    if (argc != 5 && !(argc == 7 && std::string(argv[5]) == "--output")) {
        throw std::invalid_argument(
            "chunk expects <seed> <chunk-x> <chunk-z> [--output <file>]");
    }

    const int chunkX = parseChunk(argv[3], "chunk-x");
    const int chunkZ = parseChunk(argv[4], "chunk-z");
    const mcpe::WorldGenerator world(parseSeed(argv[2]));
    std::string outputPath;

    if (argc == 7) {
        outputPath = argv[6];
        std::ofstream output(outputPath.c_str(), std::ios::binary);
        if (!output) {
            throw std::runtime_error("could not open output file");
        }
        output.write(
            reinterpret_cast<const char*>(world.chunkData(chunkX, chunkZ)),
            static_cast<std::streamsize>(mcpe::WorldGenerator::kChunkBlockCount));
        if (!output) {
            throw std::runtime_error("could not write complete chunk");
        }
    }

    std::cout << "{\"version\":\"0.6.1\",\"seed\":" << world.seed()
              << ",\"chunk\":{\"x\":" << chunkX << ",\"z\":" << chunkZ << '}'
              << ",\"generated\":" << (world.isGeneratedChunk(chunkX, chunkZ) ? "true" : "false")
              << ",\"outside_world\":" << (world.isGeneratedChunk(chunkX, chunkZ) ? "false" : "true")
              << ",\"bytes\":" << mcpe::WorldGenerator::kChunkBlockCount
              << ",\"layout\":\"x<<11|z<<7|y\""
              << ",\"fnv1a64\":" << world.chunkHash(chunkX, chunkZ)
              << ",\"diamonds\":" << world.countDiamonds(chunkX, chunkZ);
    if (!outputPath.empty()) {
        std::cout << ",\"output\":" << jsonString(outputPath);
    }
    std::cout << "}\n";
    return 0;
}

void printDiamonds(const std::vector<mcpe::DiamondBlock>& blocks) {
    std::cout << '[';
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (i != 0) {
            std::cout << ',';
        }
        std::cout << "{\"x\":" << blocks[i].x
                  << ",\"y\":" << blocks[i].y
                  << ",\"z\":" << blocks[i].z << '}';
    }
    std::cout << ']';
}

int runDiamonds(int argc, char** argv) {
    if (argc != 3 && argc != 5) {
        throw std::invalid_argument(
            "diamonds expects <seed> [<chunk-x> <chunk-z>]");
    }
    const mcpe::WorldGenerator world(parseSeed(argv[2]));
    std::vector<mcpe::DiamondBlock> blocks;
    std::cout << "{\"version\":\"0.6.1\",\"seed\":" << world.seed();
    if (argc == 5) {
        const int chunkX = parseChunk(argv[3], "chunk-x");
        const int chunkZ = parseChunk(argv[4], "chunk-z");
        blocks = world.diamonds(chunkX, chunkZ);
        std::cout << ",\"chunk\":{\"x\":" << chunkX << ",\"z\":" << chunkZ << '}';
    } else {
        blocks = world.diamonds();
    }
    std::cout << ",\"count\":" << blocks.size() << ",\"blocks\":";
    printDiamonds(blocks);
    std::cout << "}\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2 || std::string(argv[1]) == "--help" ||
            std::string(argv[1]) == "-h") {
            usage(argc < 2 ? std::cerr : std::cout);
            return argc < 2 ? 2 : 0;
        }

        const std::string command = argv[1];
        if (command == "count") {
            return runCount(argc, argv);
        }
        if (command == "chunk") {
            return runChunk(argc, argv);
        }
        if (command == "diamonds") {
            return runDiamonds(argc, argv);
        }
        throw std::invalid_argument("unknown command: " + command);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
