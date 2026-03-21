#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <string>

class Memory {
private:
    uint32_t size;        // 内存总大小
    uint8_t* data;        // 真正存数据的“大数组” (按字节存储)
    uint32_t base_addr;   // 物理内存的起始地址 (通常是 0x80000000)

public:
    // 构造函数：默认分配 1MB 内存
    Memory(uint32_t mem_size = 1024 * 1024, uint32_t base = 0x80000000);
    ~Memory(); // 析构函数：释放内存

    // 核心读写接口：读写 32 位 (4个字节) 的数据
    uint32_t read32(uint32_t addr);
    void write32(uint32_t addr, uint32_t value);

    // 核心读写接口：读写 8 位 (1个字节) 的数据 (为后续 Load/Store 指令做准备)
    uint8_t read8(uint32_t addr);
    void write8(uint32_t addr, uint8_t value);

    // 程序加载器：把编译好的二进制文件读进内存
    bool load_binary(const std::string& filename);
};

#endif