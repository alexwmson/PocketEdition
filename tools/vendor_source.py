#!/usr/bin/env python3
"""Extract the MCPE 0.6.1 headless-server source closure used by the census tool."""

from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path


def dedicated_sources(cmake_file: Path) -> list[Path]:
    text = cmake_file.read_text(encoding="utf-8")
    match = re.search(r"set\(CompileFiles\s+(.*?)\)", text, re.DOTALL)
    if not match:
        raise RuntimeError(f"CompileFiles not found in {cmake_file}")
    return [
        Path(item)
        for item in match.group(1).split()
        if not item.endswith("main.cpp") and not item.endswith("main_dedicated.cpp")
    ]


def copy_file(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source",
        type=Path,
        required=True,
        help="Path to SourceCode/handheld",
    )
    parser.add_argument(
        "--destination",
        type=Path,
        required=True,
        help="Destination directory (normally vendor/mcpe)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Replace an existing destination (local adaptations will be lost)",
    )
    args = parser.parse_args()

    source = args.source.resolve()
    destination = args.destination.resolve()
    source_root = source / "src"
    project = source / "project" / "dedicated_server"

    if destination.exists():
        if not args.force:
            raise RuntimeError(
                f"{destination} already exists; pass --force to replace it"
            )
        shutil.rmtree(destination)

    # Keep all headers/support files because the 2013 code has a broad,
    # header-heavy include graph. Only the dedicated headless .cpp files are
    # copied; client renderer and platform entry-point implementations are not.
    for path in source_root.rglob("*"):
        if path.is_file() and path.suffix.lower() != ".cpp":
            copy_file(path, destination / "src" / path.relative_to(source_root))

    compiled_sources: list[str] = []
    for relative in dedicated_sources(project / "CMakeLists.txt"):
        path = (project / relative).resolve()
        destination_path = destination / "src" / path.relative_to(source_root)
        copy_file(path, destination_path)
        compiled_sources.append(destination_path.relative_to(destination).as_posix())

    raknet = source / "project" / "lib_projects" / "raknet" / "jni" / "RaknetSources"
    for path in raknet.rglob("*"):
        if path.is_file():
            copy_file(path, destination / "raknet" / path.relative_to(raknet))

    source_manifest = "set(MCPE_LEGACY_SOURCES\n"
    source_manifest += "".join(f'    "${{MCPE_VENDOR_ROOT}}/{path}"\n' for path in compiled_sources)
    source_manifest += ")\n"
    (destination / "sources.cmake").write_text(source_manifest, encoding="utf-8")

    print(f"Extracted MCPE sources to {destination}")


if __name__ == "__main__":
    main()
