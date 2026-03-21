#include "cpu.h"
// 这里假设你们的 memory.h 里定义了一个 Memory 类，并且有一个 read32 的方法
#include "memory.h" 
#include <iomanip> // 用于格式化输出寄存器

// 1. 构造函数：CPU 开机时的初始化
CPU::CPU(Memory* memory) {
    this->mem = memory;
    // 初始化所有寄存器为 0
    for (int i = 0; i < 32; i++) {
        regs[i] = 0;
    }
    // 关键：在 RISC-V 架构中，裸机程序通常从 0x80000000 这个物理地址开始运行
    this->pc = 0x80000000; 
}

// 2. 取指 (Fetch)：从内存中当前 PC 的位置读出 32 位（4字节）的机器码
uint32_t CPU::fetch() {
    return mem->read32(this->pc);
}

// 3. 核心步进逻辑：取指 -> 更新PC -> 译码执行
void CPU::step() {
    uint32_t inst = fetch(); // 拿到指令
    this->pc += 4;           // PC 自动指向下一条指令 (32位=4字节)
    decode_and_execute(inst); // 解析并执行这条指令
}

// 4. 译码与执行 (Decode & Execute)：最硬核的部分
void CPU::decode_and_execute(uint32_t inst) {
    // 步骤 A：提取最基础的指令片段 (利用位移 >> 和按位与 &)
    uint32_t opcode = inst & 0x7F;             // 最低 7 位是操作码
    uint32_t rd     = (inst >> 7) & 0x1F;      // 目标寄存器索引 (5位)
    uint32_t funct3 = (inst >> 12) & 0x7;      // 3位功能码
    uint32_t rs1    = (inst >> 15) & 0x1F;     // 源寄存器 1 索引 (5位)
    uint32_t rs2    = (inst >> 20) & 0x1F;     // 源寄存器 2 索引 (5位)
    uint32_t funct7 = (inst >> 25) & 0x7F;     // 7位功能码

    // 步骤 B：根据 opcode 决定这是什么类型的指令
    switch (opcode) {
        // --- R 型指令 (Register-to-Register) ---
        // 比如加减法、逻辑运算等，源操作数都在寄存器里
        case 0x33: 
            if (funct3 == 0x0) {
                if (funct7 == 0x00) {
                    // 这是一条 ADD (加法) 指令！
                    // 逻辑：把 rs1 和 rs2 里的值相加，存入 rd 对应的寄存器
                    uint32_t val1 = regs[rs1];
                    uint32_t val2 = regs[rs2];
                    
                    // RISC-V 规定：x0 (regs[0]) 寄存器永远是 0，不能被修改
                    if (rd != 0) { 
                        regs[rd] = val1 + val2;
                    }
                    std::cout << "[执行] ADD x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                } 
                else if (funct7 == 0x20) {
                    // 这是一条 SUB (减法) 指令
                    uint32_t val1 = regs[rs1];
                    uint32_t val2 = regs[rs2];

                    if(rd != 0){
                        regs[rd] = val1 - val2;
                    }
                    std::cout <<"[执行]SUB x"<< rd << ", x"<< rs1<<", x"<<rs2<< std::endl;
                }
            }
            // TODO: 其他 funct3 对应的指令（如 XOR, OR, AND 等）
            break;

        // --- I 型指令 (立即数运算) ---
        case 0x13:
            if (funct3 == 0x0) { // 这是 ADDI 指令
                // 提取指令高 12 位的数字作为常数 (立即数)
                int32_t imm = ((int32_t)inst) >> 20; 
                if (rd != 0) {
                    regs[rd] = regs[rs1] + imm; // 寄存器 + 常数
                }
                std::cout << "[执行] ADDI x" << rd << ", x" << rs1 << ", " << imm << std::endl;
            }
            break;

        // 如果遇到不认识的机器码，直接报错停机
        default:
            std::cerr << "[错误] 遇到未知指令: 0x" << std::hex << inst << std::endl;
            exit(1); 
    }
}

// 5. 调试打印工具
void CPU::dump_registers() {
    std::cout << "--- 寄存器状态 ---" << std::endl;
    for (int i = 0; i < 32; i++) {
        std::cout << "x" << std::setw(2) << std::setfill('0') << std::dec << i 
                  << " = 0x" << std::hex << std::setw(8) << regs[i] << "  ";
        if ((i + 1) % 4 == 0) std::cout << std::endl; // 每行打印4个
    }
    std::cout << "PC  = 0x" << std::hex << std::setw(8) << pc << std::endl;
    std::cout << "------------------" << std::endl;
}