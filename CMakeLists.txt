cmake_minimum_required(VERSION 3.16)
project(mcpe_census LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(MCPE_VENDOR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mcpe")
include("${MCPE_VENDOR_ROOT}/sources.cmake")

set(RAKNET_SOURCES
    "${MCPE_VENDOR_ROOT}/raknet/BitStream.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/DS_HuffmanEncodingTree.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/GetTime.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/Itoa.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/LinuxStrings.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/RakMemoryOverride.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/RakNetTypes.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/RakString.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/SimpleMutex.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/SocketLayer.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/StringCompressor.cpp"
    "${MCPE_VENDOR_ROOT}/raknet/SuperFastHash.cpp")

add_library(raknet STATIC ${RAKNET_SOURCES})
target_include_directories(raknet PUBLIC "${MCPE_VENDOR_ROOT}/raknet")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(raknet PRIVATE -Wno-narrowing)
endif()

add_library(mcpe-generator STATIC
    src/generator/WorldGenerator.cpp
    ${MCPE_LEGACY_SOURCES})
target_compile_definitions(mcpe-generator PRIVATE STANDALONE_SERVER CENSUS_SILENT)
target_compile_options(mcpe-generator PRIVATE -ffunction-sections -fdata-sections)
target_include_directories(mcpe-generator
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PRIVATE "${MCPE_VENDOR_ROOT}/src" "${MCPE_VENDOR_ROOT}/raknet")
target_link_libraries(mcpe-generator PRIVATE raknet)

add_executable(mcpe-census src/main.cpp)
target_link_libraries(mcpe-census PRIVATE mcpe-generator)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(mcpe-census PRIVATE -Wl,--gc-sections)
endif()

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(mcpe-generator PUBLIC Threads::Threads)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(mcpe-census-tests tests/generator_tests.cpp)
    target_link_libraries(mcpe-census-tests PRIVATE mcpe-generator)
    target_include_directories(mcpe-census-tests PRIVATE "${MCPE_VENDOR_ROOT}/src")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(mcpe-census-tests PRIVATE -Wl,--gc-sections)
    endif()
    add_test(NAME generator-tests COMMAND mcpe-census-tests)
    add_test(
        NAME cli-export-test
        COMMAND "${CMAKE_COMMAND}"
            "-DMCPE_CENSUS=$<TARGET_FILE:mcpe-census>"
            "-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/seed0-chunk-2-0.bin"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli.cmake")
    add_test(
        NAME cli-range-test
        COMMAND "${CMAKE_COMMAND}"
            "-DMCPE_CENSUS=$<TARGET_FILE:mcpe-census>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_range.cmake")
endif()
