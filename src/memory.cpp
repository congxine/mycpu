#include "memory.h"

Memory::Memory(uint32_t base, uint32_t size)
    : base_(base), size_(size), data_(size, 0) {}

bool Memory::contains(uint32_t addr) const {
    return addr >= base_ && addr < base_ + size_;
}

uint32_t Memory::offset(uint32_t addr) const {
    if (!contains(addr)) {
        throw std::runtime_error("Memory access out of range");
    }
    return addr - base_;
}

uint8_t Memory::load8(uint32_t addr) const {
    return data_[offset(addr)];
}

uint16_t Memory::load16(uint32_t addr) const {
    uint32_t off = offset(addr);
    return static_cast<uint16_t>(
        static_cast<uint16_t>(data_[off]) |
        (static_cast<uint16_t>(data_[off + 1]) << 8)
    );
}

uint32_t Memory::load32(uint32_t addr) const {
    uint32_t off = offset(addr);
    return static_cast<uint32_t>(
        static_cast<uint32_t>(data_[off]) |
        (static_cast<uint32_t>(data_[off + 1]) << 8) |
        (static_cast<uint32_t>(data_[off + 2]) << 16) |
        (static_cast<uint32_t>(data_[off + 3]) << 24)
    );
}

void Memory::store8(uint32_t addr, uint8_t value) {
    data_[offset(addr)] = value;
}

void Memory::store16(uint32_t addr, uint16_t value) {
    uint32_t off = offset(addr);
    data_[off]     = static_cast<uint8_t>(value & 0xff);
    data_[off + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
}

void Memory::store32(uint32_t addr, uint32_t value) {
    uint32_t off = offset(addr);
    data_[off]     = static_cast<uint8_t>(value & 0xff);
    data_[off + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
    data_[off + 2] = static_cast<uint8_t>((value >> 16) & 0xff);
    data_[off + 3] = static_cast<uint8_t>((value >> 24) & 0xff);
}

void Memory::load_blob(uint32_t addr, const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        store8(addr + static_cast<uint32_t>(i), data[i]);
    }
}
