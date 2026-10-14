# Vendored MCPE source

This directory is an extracted source snapshot from
`SourceCode/handheld` for the MCPE 0.6.1 generator.

- `src/` contains the headless dedicated-server source/header closure.
- `raknet/` contains the bundled RakNet source; CMake compiles only the small
  serialization/platform subset required by the legacy object graph.
- `sources.cmake` is the explicit list of MCPE translation units.

Local portability adaptations are documented in
`../../docs/GENERATOR_NOTES.md`.

The source archive does not provide an open-source license grant for Minecraft
code. Original notices are retained where present. Do not assume this directory
is freely redistributable.
