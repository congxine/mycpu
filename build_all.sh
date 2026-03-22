#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

command -v cmake >/dev/null 2>&1 || { echo "cmake not found"; exit 1; }
command -v riscv64-unknown-elf-gcc >/dev/null 2>&1 || { echo "riscv64-unknown-elf-gcc not found"; exit 1; }

cmake -S . -B build
cmake --build build -j

riscv64-unknown-elf-gcc \
  -march=rv32i -mabi=ilp32 \
  -ffreestanding -nostdlib \
  -O1 -fno-inline \
  -Wl,-T,fw/link.ld \
  -Wl,-Map=fw/maze_game.map \
  -o fw/maze_game.elf \
  fw/start.S fw/maze_game.c

echo "Build finished:"
echo "  simulator: build/rv32i_sim"
echo "  firmware : fw/maze_game.elf"
