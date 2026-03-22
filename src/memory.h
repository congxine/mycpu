#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

class Memory {
public:
    Memory(uint32_t base, uint32_t size);

    bool contains(uint32_t addr) const;

    uint8_t  load8(uint32_t addr) const;
    uint16_t load16(uint32_t addr) const;
    uint32_t load32(uint32_t addr) const;

    void store8(uint32_t addr, uint8_t value);
    void store16(uint32_t addr, uint16_t value);
    void store32(uint32_t addr, uint32_t value);

    void load_blob(uint32_t addr, const std::vector<uint8_t>& data);

private:
    uint32_t base_;
    uint32_t size_;
    std::vector<uint8_t> data_;

    uint32_t offset(uint32_t addr) const;
};
