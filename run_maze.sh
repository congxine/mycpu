#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

echo "[1/3] Checking tools..."
command -v cmake >/dev/null 2>&1 || { echo "cmake not found"; exit 1; }
command -v riscv64-unknown-elf-gcc >/dev/null 2>&1 || { echo "riscv64-unknown-elf-gcc not found"; exit 1; }

echo "[2/3] Building simulator..."
cmake -S . -B build
cmake --build build -j

echo "[3/3] Building maze firmware..."
riscv64-unknown-elf-gcc \
  -march=rv32i -mabi=ilp32 \
  -ffreestanding -nostdlib \
  -O1 -fno-inline \
  -Wl,-T,fw/link.ld \
  -Wl,-Map=fw/maze_game.map \
  -o fw/maze_game.elf \
  fw/start.S fw/maze_game.c

echo "Running maze game..."
exec ./build/rv32i_sim --raw-kbd fw/maze_game.elf
