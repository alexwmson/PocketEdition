#include "generator/WorldGenerator.h"

#include <cctype>
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
        << "  mcpe-census count --range <start>-<end> [--per-chunk]\n"
        << "  mcpe-census chunk <seed> <chunk-x> <chunk-z> [--output <file>]\n"
        << "  mcpe-census diamonds <seed> [<chunk-x> <chunk-z>]\n"
        << "\n"
        << "Seeds are signed 32-bit integers. A count range is inclusive and\n"
        << "runs in one process, printing one JSON object per seed. Negative\n"
        << "bounds keep their sign, for example -10--1 or -10..-1.\n"
        << "Queryable chunk coordinates are 0..25; only 0..15 contain\n"
        << "generated MCPE finite-world terrain.\n";
}

struct SeedSpan {
    std::int32_t start;
    std::int32_t end;
};

long long parseInteger(const char* text, const char* name);

std::int32_t requireSeed(long long value, const char* name) {
    if (value < INT32_MIN || value > INT32_MAX) {
        throw std::out_of_range(std::string(name) + " must be in signed 32-bit range");
    }
    return static_cast<std::int32_t>(value);
}

std::size_t rangeSeparator(const std::string& spec, std::size_t* length) {
    const std::size_t dots = spec.find("..");
    if (dots != std::string::npos) {
        if (spec.find("..", dots + 2) != std::string::npos) {
            throw std::invalid_argument("range must contain one start-end pair");
        }
        *length = 2;
        return dots;
    }

    std::size_t index = 0;
    if (!spec.empty() && (spec[0] == '+' || spec[0] == '-')) {
        ++index;
    }
    while (index < spec.size() &&
           std::isdigit(static_cast<unsigned char>(spec[index]))) {
        ++index;
    }
    if (index == 0 || index >= spec.size() || spec[index] != '-') {
        throw std::invalid_argument("range must look like 0-9 or -10--1");
    }
    *length = 1;
    return index;
}

SeedSpan parseSeedRange(const std::string& spec) {
    std::size_t separatorLength = 0;
    const std::size_t separator = rangeSeparator(spec, &separatorLength);
    if (separator == 0 || separator + separatorLength >= spec.size()) {
        throw std::invalid_argument("range must look like 0-9 or -10--1");
    }

    const long long start = parseInteger(
        spec.substr(0, separator).c_str(), "range start");
    const long long end = parseInteger(
        spec.substr(separator + separatorLength).c_str(), "range end");
    if (start > end) {
        throw std::invalid_argument("range start must not exceed range end");
    }

    SeedSpan span;
    span.start = requireSeed(start, "range start");
    span.end = requireSeed(end, "range end");
    return span;
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
    return requireSeed(parseInteger(text, "seed"), "seed");
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

void printCount(const mcpe::WorldGenerator& world, bool perChunk) {
    std::cout << "{\"version\":\"0.6.1\",\"seed\":" << world.seed()
              << ",\"diamond_block_id\":56"
              << ",\"diamonds\":" << world.countDiamonds();
    if (perChunk) {
        printChunkCounts(world);
    }
    std::cout << "}\n" << std::flush;
}

int runCount(int argc, char** argv) {
    bool perChunk = false;
    bool sawSeed = false;
    bool sawRange = false;
    SeedSpan span = {0, 0};

    for (int i = 2; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--per-chunk") {
            if (perChunk) {
                throw std::invalid_argument("count received --per-chunk twice");
            }
            perChunk = true;
            continue;
        }

        std::string rangeSpec;
        if (arg == "--range") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("count --range expects <start>-<end>");
            }
            rangeSpec = argv[++i];
        } else if (arg.compare(0, 8, "--range=") == 0) {
            rangeSpec = arg.substr(8);
        } else if (arg.compare(0, 2, "--") == 0) {
            throw std::invalid_argument("unknown count option: " + arg);
        } else {
            if (sawSeed || sawRange) {
                throw std::invalid_argument(
                    "count expects <seed> [--per-chunk] or --range <start>-<end> [--per-chunk]");
            }
            span.start = parseSeed(argv[i]);
            span.end = span.start;
            sawSeed = true;
            continue;
        }

        if (sawSeed || sawRange) {
            throw std::invalid_argument("count received more than one seed range");
        }
        if (rangeSpec.empty()) {
            throw std::invalid_argument("count --range expects <start>-<end>");
        }
        span = parseSeedRange(rangeSpec);
        sawRange = true;
    }

    if (!sawSeed && !sawRange) {
        throw std::invalid_argument(
            "count expects <seed> [--per-chunk] or --range <start>-<end> [--per-chunk]");
    }

    for (long long seed = span.start; seed <= span.end; ++seed) {
        const mcpe::WorldGenerator world(static_cast<std::int32_t>(seed));
        printCount(world, perChunk);
    }
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
