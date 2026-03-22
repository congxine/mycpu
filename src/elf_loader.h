#pragma once
#include <cstdint>
#include <string>

class Bus;

struct LoadedImage {
    uint32_t entry = 0;
};

bool is_elf_file(const std::string& path);
LoadedImage load_elf_into_bus(const std::string& path, Bus& bus);
