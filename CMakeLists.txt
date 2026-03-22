cmake_minimum_required(VERSION 3.16)
project(rv32i_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(rv32i_sim
    src/main.cpp
    src/cpu.cpp
    src/bus.cpp
    src/memory.cpp
    src/elf_loader.cpp
)
