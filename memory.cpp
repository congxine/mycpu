#include "memory.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

Memory::Memory(uint32_t mem_size, uint32_t base) {
    this->size = mem_size;
    this->base_addr = base;
    this->data = new uint8_t[size]; // 向电脑申请 1MB 的真内存
    
    // 开机时，把内存全清零
    for (uint32_t i = 0; i < size; i++) {
        data[i] = 0;
    }
}

Memory::~Memory() {
    delete[] data; // 关机拔内存条
}

// -----------------------------------------
// 读写 8 位 (单字节)
// -----------------------------------------
uint8_t Memory::read8(uint32_t addr) {
    uint32_t index = addr - base_addr; // 地址映射
    if (index >= size) {
        std::cerr << "[内存错误] 读地址越界: 0x" << std::hex << addr << std::endl;
        exit(1);
    }
    return data[index];
}

void Memory::write8(uint32_t addr, uint8_t value) {
    uint32_t index = addr - base_addr;
    if (index >= size) {
        std::cerr << "[内存错误] 写地址越界: 0x" << std::hex << addr << std::endl;
        exit(1);
    }
    data[index] = value;
}

// -----------------------------------------
// 读写 32 位 (4字节) - 注意小端序拼装
// -----------------------------------------
uint32_t Memory::read32(uint32_t addr) {
    // 连续读 4 个字节
    uint32_t b0 = read8(addr);
    uint32_t b1 = read8(addr + 1);
    uint32_t b2 = read8(addr + 2);
    uint32_t b3 = read8(addr + 3);
    
    // 拼装成一个完整的 32 位数字 (低地址在低位)
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void Memory::write32(uint32_t addr, uint32_t value) {
    // 拆解成 4 个字节，依次存入
    write8(addr,     value & 0xFF);
    write8(addr + 1, (value >> 8) & 0xFF);
    write8(addr + 2, (value >> 16) & 0xFF);
    write8(addr + 3, (value >> 24) & 0xFF);
}

// -----------------------------------------
// 加载器：将 .bin 文件读入虚拟内存的开头
// -----------------------------------------
bool Memory::load_binary(const std::string& filename) {
    // 以二进制格式，把光标放在文件末尾打开 (为了直接获取文件大小)
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法打开测试文件: " << filename << std::endl;
        return false;
    }

    std::streamsize file_size = file.tellg(); // 获取文件大小
    if (file_size > size) {
        std::cerr << "[错误] 文件太大，内存装不下！" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg); // 光标移回文件开头
    // 把文件内容直接塞进我们的 data 数组里
    if (file.read(reinterpret_cast<char*>(data), file_size)) {
        std::cout << "[系统] 成功加载程序，大小: " << file_size << " 字节。" << std::endl;
        return true;
    }
    return false;
}