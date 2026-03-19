#include <iostream>
#include <cstdint>

// 定义 RISC-V 32位 架构的基本组件
struct RISCV_CPU {
    uint32_t regs[32]; // 32个通用寄存器 (x0 - x31)
    uint32_t pc;       // 程序计数器

    RISCV_CPU() : pc(0) {
        for(int i = 0; i < 32; i++) regs[i] = 0;
    }
};

int main() {
    RISCV_CPU myCPU;
    std::cout << "myCPU Simulator Initialized." << std::endl;
    std::cout << "Current PC: 0x" << std::hex << myCPU.pc << std::dec << std::endl;
    return 0;
}
